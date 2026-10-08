// Native harness: stream-out probe of deforming vertex shaders (path 1, S1).
// The mock command list plays the D3D11 stream-output stage: a draw with the
// probe's geometry shader and target bound writes, per triangle vertex,
// SV_Position = viewProj * world and the shader's other outputs into the
// target, and the statistics query reports the primitives written.
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include "gen/mock_base.hpp"
#include "harness_timer.hpp"
#include "src/games/falcomengine-plus/world/bvh/deform_probe.hpp"

using namespace reshade::api;
namespace bvh = falcom_world::bvh;
namespace contract = falcom_world::contract;

static int g_failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++g_failures; std::printf("FAIL %s:%d %s | ", __FILE__, __LINE__, #cond); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

struct Res { resource_desc desc; std::vector<uint8_t> bytes; };
struct Device : mock::DeviceBase {
  std::map<uint64_t, Res> res;
  uint64_t next = 0x1000;
  int resources_alive = 0;
  device_api get_api() const override { return device_api::d3d11; }
  bool create_resource(const resource_desc &desc, const subresource_data *, resource_usage, resource *out, void ** = nullptr) override {
    Res r; r.desc = desc; r.bytes.assign(desc.buffer.size, 0);
    out->handle = next; next += 0x100; res[out->handle] = std::move(r); ++resources_alive;
    return true;
  }
  void destroy_resource(resource r) override { if (res.erase(r.handle) != 0u) --resources_alive; }
  resource_desc get_resource_desc(resource r) const override { auto it = res.find(r.handle); return it == res.end() ? resource_desc{} : it->second.desc; }
  bool map_buffer_region(resource r, uint64_t offset, uint64_t size, map_access, void **out) override {
    auto it = res.find(r.handle); if (it == res.end() || offset + size > it->second.bytes.size()) return false;
    *out = it->second.bytes.data() + offset; return true;
  }
  void unmap_buffer_region(resource) override {}
};

// What the mock "shader" writes (per test case).
struct Scene {
  float view_proj[16] = {};
  bool transposed = false;      // the shader multiplies with the transposed matrix
  bool world_output = true;     // TEXCOORD1 holds the world position (else garbage)
  uint32_t drop_primitives = 0;  // primitives the "GPU" fails to write (overflow)
};

struct CmdList : mock::CommandListBase {
  Device* dev = nullptr;
  Scene* scene = nullptr;
  uint64_t gs = 0u;
  resource so = {0u};
  uint64_t so_offset = 0u;
  uint64_t last_written = 0u;
  int binds_under_lock = 0;
  std::vector<std::string> calls;
  device *get_device() override { return dev; }
  void probe_lock() {
    if (bvh::g_deform.mutex.try_lock()) bvh::g_deform.mutex.unlock();
    else ++binds_under_lock;
  }
  void bind_pipeline(pipeline_stage stages, pipeline p) override {
    probe_lock();
    if (stages == (pipeline_stage::geometry_shader | pipeline_stage::stream_output)) {
      gs = p.handle;
      calls.push_back(p.handle != 0u ? "gs" : "gs0");
    }
  }
  void bind_stream_output_buffers(uint32_t first, uint32_t count, const resource *buffers, const uint64_t *offsets, const uint64_t *, const resource *, const uint64_t *) override {
    probe_lock();
    if (first == 0u && count == 1u) { so = buffers[0]; so_offset = offsets[0]; }
    calls.push_back(so.handle != 0u ? "so" : "so0");
  }
  uint64_t last_needed = 0u;
  int indirect_calls = 0;
  // Writes `primitives` triangles (as the SO stage would), stopping at the end of the buffer.
  void emit(uint64_t primitives, uint64_t dropped) {
    last_written = 0u;
    last_needed = 0u;
    if (gs == 0u || so.handle == 0u) return;
    // Layout of 0x51654E2F: SV_Position xyzw, NORMAL0 xyz, TEXCOORD1 xyzw, TEXCOORD2 xyzw, TEXCOORD5 xyzw, TEXCOORD6 xyzw.
    const uint32_t stride_floats = 4 + 3 + 4 + 4 + 4 + 4;
    const uint64_t stride = stride_floats * 4u;
    auto& bytes = dev->res[so.handle].bytes;
    const uint64_t fit = so_offset < bytes.size() ? (bytes.size() - so_offset) / (stride * 3u) : 0u;
    const uint64_t written = std::min<uint64_t>(primitives - dropped, fit);
    for (uint64_t v = 0; v < written * 3u; ++v) {
      const float world[4] = {100.f + float(v % 7), 2.f + float((v * 3) % 5), -40.f + float(v / 3), 1.f};
      float clip[4] = {};
      for (int r = 0; r < 4; ++r)
        for (int k = 0; k < 4; ++k) clip[r] += world[k] * (scene->transposed ? scene->view_proj[k * 4 + r] : scene->view_proj[r * 4 + k]);
      float out[stride_floats] = {};
      std::memcpy(out, clip, 16);
      out[4] = 0.f; out[5] = 1.f; out[6] = 0.f;  // normal
      if (scene->world_output) std::memcpy(out + 7, world, 16);
      else { out[7] = clip[0] * 0.5f; out[8] = 7.f; out[9] = -3.f; out[10] = 1.f; }
      out[11] = 0.25f; out[12] = 0.5f; out[13] = 0.f; out[14] = 0.f;  // uv
      std::memcpy(out + 15, clip, 16);                                  // TEXCOORD5 = clip copy
      out[19] = clip[0] + 1.f; out[20] = clip[1]; out[21] = clip[2]; out[22] = clip[3];  // prev clip
      std::memcpy(bytes.data() + so_offset + v * stride, out, stride);
    }
    last_written = written;
    last_needed = primitives;
  }
  void draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t, int32_t, uint32_t) override {
    calls.push_back("draw");
    emit(uint64_t(index_count / 3u) * instance_count, scene->drop_primitives);
  }
  void draw_or_dispatch_indirect(indirect_command type, resource buffer, uint64_t offset, uint32_t draw_count, uint32_t stride) override {
    calls.push_back("indirect");
    ++indirect_calls;
    if (type != indirect_command::draw_indexed || draw_count != 1u || stride != 20u) { last_written = last_needed = 0u; return; }
    uint32_t args[5] = {};
    std::memcpy(args, dev->res[buffer.handle].bytes.data() + offset, sizeof(args));
    emit(uint64_t(args[0] / 3u) * args[1], 0u);
  }
  void copy_buffer_region(resource src, uint64_t so_, resource dst, uint64_t dof, uint64_t size) override {
    auto s = dev->res.find(src.handle), d = dev->res.find(dst.handle);
    if (s == dev->res.end() || d == dev->res.end() || so_ + size > s->second.bytes.size() || dof + size > d->second.bytes.size()) return;
    std::memcpy(d->second.bytes.data() + dof, s->second.bytes.data() + so_, size);
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
static void Classify(Device* dev, uint64_t handle, const std::string& file) {
  auto code = ReadFile(std::string(FALCOM_BYTECODE_DIR) + file);
  if (code.empty()) { std::printf("missing %s\n", file.c_str()); std::exit(2); }
  shader_desc desc = {}; desc.code = code.data(); desc.code_size = code.size();
  pipeline_subobject sub = {pipeline_subobject_type::vertex_shader, 1, &desc};
  contract::OnInitPipelineClassify(dev, {0}, 1, &sub, pipeline{handle});
}

// Mock backend.
static int g_created = 0, g_destroyed = 0, g_create_fail = 0;
static bvh::DeformSkip g_game_state = bvh::DeformSkip::None;
static std::set<uint64_t> g_alive_gs;
static uint64_t MockCreate(device*, const contract::ShaderCode& code, const bvh::DeformSoLayout& layout, int32_t* hr) {
  if (g_create_fail) { *hr = static_cast<int32_t>(0x80070057u); return 0u; }
  *hr = 0; ++g_created;
  const uint64_t handle = 0xC0DE0000u + g_created;
  g_alive_gs.insert(handle);
  (void)code; (void)layout;
  return handle;
}
static void MockDestroy(uint64_t shader) { ++g_destroyed; g_alive_gs.erase(shader); }
static bvh::DeformSkip MockCheck(command_list*) { return g_game_state; }
static std::map<uint64_t, std::array<uint64_t, 2>> g_queries;
static std::set<uint64_t> g_unready, g_broken;
static uint64_t g_next_query = 0xA0000;
static int g_queries_alive = 0;
static uint64_t MockCreateQuery(device*) { ++g_queries_alive; const uint64_t q = ++g_next_query; g_queries[q] = {0u, 0u}; return q; }
static void MockDestroyQuery(uint64_t q) { if (g_queries.erase(q) != 0u) --g_queries_alive; }
static uint64_t g_open_query = 0u;
static void MockBeginQuery(command_list* cl, uint64_t q) { static_cast<CmdList*>(cl)->calls.push_back("begin"); g_open_query = q; }
static void MockEndQuery(command_list* cmd, uint64_t q) {
  auto* cl = static_cast<CmdList*>(cmd);
  cl->calls.push_back("end");
  if (g_open_query == q) g_queries[q] = {cl->last_written, cl->last_needed};
  g_open_query = 0u;
}
static int MockReadQuery(command_list*, uint64_t q, uint64_t* out) {
  if (g_broken.count(q) != 0u) return -1;
  if (g_unready.count(q) != 0u) return 0;
  auto it = g_queries.find(q);
  if (it == g_queries.end()) return -1;
  out[0] = it->second[0]; out[1] = it->second[1];
  return 1;
}

int main() {
  Device dev; Scene scene; CmdList cl; cl.dev = &dev; cl.scene = &scene; Queue queue; queue.dev = &dev; queue.cl = &cl;
  CmdList deferred; deferred.dev = &dev; deferred.scene = &scene;
  bvh::g_deform.backend = {MockCreate, MockDestroy, MockCheck, MockCreateQuery, MockDestroyQuery, MockBeginQuery, MockEndQuery, MockReadQuery};
  bvh::g_pool.immediate_cmd_list.store(reinterpret_cast<uint64_t>(static_cast<command_list*>(&cl)));

  const uint64_t kSkin = 0x51A, kSkinLight = 0x51B, kRigid = 0x51C;
  Classify(&dev, kSkin, "0x51654E2F.vs.cso");
  Classify(&dev, kSkinLight, "0x0417B98E.vs.cso");
  Classify(&dev, kRigid, "0x095017A3.vs.cso");

  HStage("1. Output signature and layout from the real bytecode.");
  // 1. Output signature and layout from the real bytecode.
  {
    const auto code = contract::LookupDeformingCode(kSkin);
    CHECK(code != nullptr && contract::LookupDeformingCode(kRigid) == nullptr, "deforming code kept for skinned only");
    bvh::DeformSoLayout layout;
    const char* error = bvh::BuildDeformLayout(code->outputs, &layout);
    std::string names;
    for (auto& e : layout.elements) names += bvh::DeformElementName(e) + " ";
    std::printf("layout: %s stride %u candidates %u (%s)\n", error ? error : "ok", layout.stride, layout.candidates, names.c_str());
    CHECK(error == nullptr && layout.stride == 92u && layout.candidates == 5u && layout.position == 0, "layout");
    CHECK(names == "SV_Position0.xyzw NORMAL0.xyz TEXCOORD1.xyzw TEXCOORD2.xyzw TEXCOORD5.xyzw TEXCOORD6.xyzw ", "names");
  }

  // Camera: an asymmetric projection-like matrix (contiguous rows = convention 0).
  const float vp[16] = {1.2f, 0.1f, 0.0f, -3.f, 0.f, 1.7f, 0.2f, 5.f, 0.f, 0.f, 0.001f, 0.5f, 0.05f, 0.f, 0.9f, 60.f};
  std::memcpy(scene.view_proj, vp, sizeof(vp));
  auto set_camera = [&](uint32_t frame) {
    std::lock_guard lock(falcom_world::g_state.mutex);
    falcom_world::g_state.camera.valid = true;
    falcom_world::g_state.camera.frame = frame;
    std::memcpy(falcom_world::g_state.camera.view_proj, vp, sizeof(vp));
  };
  auto draw_for = [&](uint64_t vs, uint32_t index_count, uint32_t instances) {
    falcom_world::DrawRecord draw;
    draw.method = 1; draw.has_index_buffer = true; draw.vs_pipeline = vs; draw.index_count = index_count;
    draw.instance_count = instances; draw.topology = primitive_topology::triangle_list;
    draw.frame = falcom_world::g_state.frame.load();
    return draw;
  };
  bool camera_follows = true;
  auto present = [&]() {
    if (camera_follows) set_camera(falcom_world::g_state.frame.load());
    falcom_world::g_state.frame.fetch_add(1u);
    bvh::DrainDeformProbe(&dev, &queue);
  };
  auto stats = [&]() { bvh::DeformStats s; bvh::SnapshotDeformShaders(&s); return s; };
  auto row = [&](uint64_t pipeline) {
    for (auto& r : bvh::SnapshotDeformShaders(nullptr)) if (r.pipeline == pipeline) return r.shader;
    return bvh::DeformShader{};
  };

  HStage("2. Off: nothing happens.");
  // 2. Off: nothing happens.
  bvh::g_deform.enabled.store(false);
  bvh::OnDeformProbeDraw(&dev, &cl, draw_for(kSkin, 300, 1));
  present();
  CHECK(cl.calls.empty() && dev.resources_alive == 0 && row(kSkin).draws == 0u, "off: no work");

  HStage("3. On: resources at the first present, shader at the first draw, probe at the next.");
  // 3. On: resources at the first present, shader at the first draw, probe at the next.
  bvh::g_deform.enabled.store(true);
  present();
  CHECK(dev.resources_alive == 6 && g_queries_alive == 12, "3 slots x (so, staging) + 3 x 4 queries: %d %d", dev.resources_alive, g_queries_alive);
  bvh::OnDeformProbeDraw(&dev, &cl, draw_for(kSkin, 300, 2));
  CHECK(g_created == 1 && cl.calls.empty(), "first draw builds the shader only");
  bvh::OnDeformProbeDraw(&dev, &cl, draw_for(kSkin, 300, 2));
  CHECK((cl.calls == std::vector<std::string>{"gs", "so", "begin", "draw", "end", "so0", "gs0"}), "bind/draw/unbind order (%zu calls)", cl.calls.size());
  CHECK(cl.gs == 0u && cl.so.handle == 0u, "state restored");
  CHECK(cl.binds_under_lock == 0, "no graphics call under the lock");
  bvh::OnDeformProbeDraw(&dev, &cl, draw_for(kSkin, 300, 2));
  CHECK(row(kSkin).skips[(size_t)bvh::DeformSkip::Interval] == 1u, "interval");
  present();
  CHECK(stats().reads == 0u, "not read before two presents");
  present();
  present();
  {
    const auto s = stats();
    const auto shader = row(kSkin);
    std::printf("probe: probes %llu reads %llu matched %llu, world %s error %.3g convention %u tris %u bbox (%.1f %.1f %.1f)..(%.1f %.1f %.1f)\n",
                (unsigned long long)s.probes, (unsigned long long)s.reads, (unsigned long long)s.matched,
                bvh::DeformChosenText(shader).c_str(), shader.worst_error, shader.convention, shader.last.triangles,
                shader.last.bbox_min[0], shader.last.bbox_min[1], shader.last.bbox_min[2], shader.last.bbox_max[0], shader.last.bbox_max[1], shader.last.bbox_max[2]);
    CHECK(s.probes == 1u && s.reads == 1u && s.matched == 1u, "one probe matched");
    CHECK(bvh::DeformChosenText(shader) == "TEXCOORD1.xyzw" && shader.convention == 0u && shader.worst_error < 1e-5f, "world output found");
    CHECK(shader.last.triangles == 200u && shader.last.vertices == 600u && shader.last.nonfinite == 0u, "counts (2 instances x 100)");
    CHECK(shader.last.bbox_min[0] == 100.f && shader.last.bbox_max[0] == 106.f && shader.last.bbox_min[2] == -40.f, "bounds");
    CHECK(shader.last.candidates.size() == 6u && shader.last.candidates[4].error[0] > 1.f, "TEXCOORD5 (clip copy) rejected");
  }

  HStage("4. Light-view, rigid and deferred draws.");
  // 4. Light-view, rigid and deferred draws.
  cl.calls.clear();
  bvh::OnDeformProbeDraw(&dev, &cl, draw_for(kSkinLight, 300, 1));
  bvh::OnDeformProbeDraw(&dev, &cl, draw_for(kRigid, 300, 1));
  for (int i = 0; i < 40; ++i) present();
  bvh::OnDeformProbeDraw(&dev, &deferred, draw_for(kSkin, 300, 1));
  CHECK(cl.calls.empty() && deferred.calls.empty(), "no probe");
  CHECK(row(kSkinLight).skips[(size_t)bvh::DeformSkip::LightView] == 1u, "light view");
  CHECK(row(kSkin).skips[(size_t)bvh::DeformSkip::Deferred] == 1u, "deferred");
  CHECK(bvh::SnapshotDeformShaders(nullptr).size() == 2u, "rigid VS not tracked");

  HStage("5. Game state in the way: nothing bound, reason counted.");
  // 5. Game state in the way: nothing bound, reason counted.
  g_game_state = bvh::DeformSkip::GameGeometryShader;
  bvh::OnDeformProbeDraw(&dev, &cl, draw_for(kSkin, 300, 1));
  CHECK(cl.calls.empty() && row(kSkin).skips[(size_t)bvh::DeformSkip::GameGeometryShader] == 1u, "game GS");
  g_game_state = bvh::DeformSkip::None;

  HStage("6. Overflow (fewer primitives written) and a camera from another frame.");
  // 6. Overflow (fewer primitives written) and a camera from another frame.
  scene.drop_primitives = 1;
  bvh::OnDeformProbeDraw(&dev, &cl, draw_for(kSkin, 300, 1));
  for (int i = 0; i < 3; ++i) present();
  scene.drop_primitives = 0;
  CHECK(row(kSkin).read_fails[(size_t)bvh::DeformReadFail::QueryMismatch] == 1u, "query mismatch");
  for (int i = 0; i < 30; ++i) present();
  camera_follows = false;
  bvh::OnDeformProbeDraw(&dev, &cl, draw_for(kSkin, 300, 1));
  for (int i = 0; i < 3; ++i) present();
  camera_follows = true;
  CHECK(row(kSkin).read_fails[(size_t)bvh::DeformReadFail::CameraFrame] == 1u, "camera frame");

  HStage("7. No world output; then not-ready statistics.");
  // 7. No world output; then not-ready statistics.
  for (int i = 0; i < 30; ++i) present();
  scene.world_output = false;
  bvh::OnDeformProbeDraw(&dev, &cl, draw_for(kSkin, 300, 1));
  for (int i = 0; i < 3; ++i) present();
  scene.world_output = true;
  CHECK(row(kSkin).read_fails[(size_t)bvh::DeformReadFail::NoMatch] == 1u, "no match");
  {
    const auto before = row(kSkin);
    for (int i = 0; i < 30; ++i) present();
    for (auto& [q, v] : g_queries) g_unready.insert(q);
    bvh::OnDeformProbeDraw(&dev, &cl, draw_for(kSkin, 300, 1));
    for (int i = 0; i < 3; ++i) present();
    g_unready.clear();
    const auto after = row(kSkin);
    CHECK(after.read_fails[(size_t)bvh::DeformReadFail::QueryNotReady] == 1u && after.reads == before.reads, "not ready: not judged");
    for (int i = 0; i < 30; ++i) present();
    for (auto& [q, v] : g_queries) g_broken.insert(q);
    bvh::OnDeformProbeDraw(&dev, &cl, draw_for(kSkin, 300, 1));
    for (int i = 0; i < 3; ++i) present();
    g_broken.clear();
    CHECK(row(kSkin).read_fails[(size_t)bvh::DeformReadFail::QueryError] == 1u, "query error");
  }

  HStage("8. Transposed convention (direct evaluation).");
  // 8. Transposed convention (direct evaluation).
  {
    scene.transposed = true;
    for (int i = 0; i < 30; ++i) present();
    bvh::OnDeformProbeDraw(&dev, &cl, draw_for(kSkin, 30, 1));
    for (int i = 0; i < 3; ++i) present();
    scene.transposed = false;
    CHECK(row(kSkin).convention == 1u && row(kSkin).last.chosen == 2, "transposed found");
  }

  HStage("9. Too large for a slot; create failure for another shader.");
  // 9. Too large for a slot; create failure for another shader.
  {
    for (int i = 0; i < 30; ++i) present();
    bvh::OnDeformProbeDraw(&dev, &cl, draw_for(kSkin, 3u * 40000u, 4));  // 480k vertices x 92 B
    CHECK(row(kSkin).skips[(size_t)bvh::DeformSkip::TooLarge] == 1u, "too large");
    const uint64_t kWind = 0x51D;
    Classify(&dev, kWind, "0x9FF8E4BE.vs.cso");
    g_create_fail = 1;
    bvh::OnDeformProbeDraw(&dev, &cl, draw_for(kWind, 30, 1));
    bvh::OnDeformProbeDraw(&dev, &cl, draw_for(kWind, 30, 1));
    g_create_fail = 0;
    const auto wind = row(kWind);
    CHECK(wind.create_failed && wind.create_hr == static_cast<int32_t>(0x80070057u) && wind.skips[(size_t)bvh::DeformSkip::ShaderCreateFailed] == 2u, "create failure kept");
  }

  HStage("9b. Indirect draws: the capture takes the rest of the slot; the size comes from the statistics.");
  // 9b. Indirect draws: the capture takes the rest of the slot; the size comes from the statistics.
  {
    const uint64_t kSkinIndirect = 0x51E;
    Classify(&dev, kSkinIndirect, "0x51654E2F.vs.cso");
    resource args;
    dev.create_resource(resource_desc(64, memory_heap::gpu_only, resource_usage::indirect_argument), nullptr, resource_usage::general, &args);
    auto set_args = [&](uint32_t index_count, uint32_t instances) {
      const uint32_t a[5] = {index_count, instances, 0u, 0u, 0u};
      std::memcpy(dev.res[args.handle].bytes.data(), a, sizeof(a));
    };
    auto indirect_draw = [&](uint64_t vs) {
      falcom_world::DrawRecord draw = draw_for(vs, 0u, 0u);
      bvh::OnDeformProbeIndirectDraw(&dev, &cl, draw, args, 0u, 1u, 0u);
    };
    for (int i = 0; i < 30; ++i) present();
    indirect_draw(kSkinIndirect);  // builds the shader
    CHECK(cl.indirect_calls == 0, "no draw on the build call");
    set_args(300, 1);
    indirect_draw(kSkinIndirect);
    CHECK(cl.indirect_calls == 1, "indirect re-issued");
    bvh::OnDeformProbeDraw(&dev, &cl, draw_for(kSkin, 30, 1));
    CHECK(row(kSkin).skips[(size_t)bvh::DeformSkip::SlotFull] == 1u, "slot closed after an indirect capture");
    for (int i = 0; i < 3; ++i) present();
    auto shader = row(kSkinIndirect);
    CHECK(shader.matched == 1u && shader.last_indirect && shader.last.triangles == 100u && bvh::DeformChosenText(shader) == "TEXCOORD1.xyzw",
          "indirect matched (%u, tris %u)", shader.matched, shader.last.triangles);
    // Empty args: not judged, and the shader may probe again right away.
    for (int i = 0; i < 30; ++i) present();
    set_args(300, 0);
    indirect_draw(kSkinIndirect);
    for (int i = 0; i < 3; ++i) present();
    set_args(300, 1);
    indirect_draw(kSkinIndirect);
    shader = row(kSkinIndirect);
    CHECK(shader.read_fails[(size_t)bvh::DeformReadFail::Empty] == 1u && shader.reads == 1u && shader.probes == 3u,
          "empty then re-probed (empty %u reads %u probes %u)", shader.read_fails[(size_t)bvh::DeformReadFail::Empty], shader.reads, shader.probes);
    for (int i = 0; i < 3; ++i) present();
    CHECK(row(kSkinIndirect).matched == 2u, "second indirect matched");
    // Larger than the slot: overflow is detected, nothing judged.
    for (int i = 0; i < 30; ++i) present();
    set_args(3u * 400000u, 1);
    indirect_draw(kSkinIndirect);
    for (int i = 0; i < 3; ++i) present();
    CHECK(row(kSkinIndirect).read_fails[(size_t)bvh::DeformReadFail::QueryMismatch] == 1u && row(kSkinIndirect).matched == 2u, "overflow");
    CHECK(cl.binds_under_lock == 0, "lock rule (indirect)");
    dev.destroy_resource(args);
  }

  HStage("10. Pipeline destroyed: its shader is released at the next present; dump.");
  // 10. Pipeline destroyed: its shader is released at the next present; dump.
  {
    contract::OnDestroyPipelineClassify(&dev, pipeline{kSkin});
    bvh::OnDestroyPipelineDeform(&dev, pipeline{kSkin});
    const int destroyed = g_destroyed;
    present();
    CHECK(g_destroyed == destroyed + 1 && contract::LookupDeformingCode(kSkin) == nullptr, "shader released");
    std::filesystem::remove(bvh::PoolOutputDir() / "world_deform.json");
    bvh::DumpDeformProbe();
    std::ifstream f(bvh::PoolOutputDir() / "world_deform.json");
    const std::string text((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    CHECK(text.find("\"schema\": 2") != std::string::npos && text.find("\"shader_failures\": 1") != std::string::npos, "dump stats");
    CHECK(text.find("\"world_output\": \"-\"") != std::string::npos && text.find("\"light_view\": 1") != std::string::npos, "dump rows");
  }

  HStage("11. Re-classified handle with another bytecode resets its entry.");
  // 11. Re-classified handle with another bytecode resets its entry.
  {
    Classify(&dev, kSkin, "0x37E8915A.vs.cso");
    bvh::OnDeformProbeDraw(&dev, &cl, draw_for(kSkin, 30, 1));
    const auto shader = row(kSkin);
    CHECK(shader.probes == 0u && shader.draws == 1u, "fresh entry for new bytecode");
  }

  HStage("12. Off: everything released.");
  // 12. Off: everything released.
  bvh::g_deform.enabled.store(false);
  present();
  CHECK(dev.resources_alive == 0 && g_queries_alive == 0, "resources released (%d, queries %d)", dev.resources_alive, g_queries_alive);
  CHECK(g_alive_gs.empty(), "shaders released (%zu)", g_alive_gs.size());
  CHECK(cl.binds_under_lock == 0, "lock rule");

  std::printf("%s (%d failures)\n", g_failures == 0 ? "PASS" : "FAILED", g_failures);
  return g_failures == 0 ? 0 : 1;
}
