// Native harness for the pool: mock device/command list execute copies
// immediately (as the GPU would at that point of the stream).
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <filesystem>
#include <string>
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

// Camera visibility (round 10): registry traits from real bytecode, per-pass
// sightings and fade inputs through the draw scan, the instance/mesh basis of
// the near fade, the fade formula, and the dump section.
struct VisObject {
  const char* name; uint64_t vb; uint64_t vs; uint64_t ps; bool color;
  std::vector<std::array<float, 3>> translations;
  std::vector<std::array<float, 4>> params;  // per translation: start, inv_range, flip, opacity
};

static const bvh::WorldInstance* FindInstance(float x) {
  for (const auto& inst : bvh::g_pool.instances) if (inst.matrix[3] == x) return &inst;
  return nullptr;
}

int main() {
  Device dev; CmdList cl; cl.dev = &dev; Queue queue; queue.dev = &dev; queue.cl = &cl;
  fixture::RegisterLayout();

  const uint64_t kRigid = 0xA1, kRigidShadow = 0xA3, kMeshVs = 0xA5, kShadowVs = 0xA6, kOtherLayout = 0xA4;
  const uint64_t kOpaque = 0xB1, kAlpha = 0xB2, kNearFadePs = 0xB3, kNearFadePs2 = 0xB4;
  Classify(&dev, kRigid, "0x095017A3.vs.cso", true);
  Classify(&dev, kRigidShadow, "0x91CE438F.vs.cso", true);
  Classify(&dev, kMeshVs, "0x153708C7.vs.cso", true);
  Classify(&dev, kShadowVs, "0xE35C18B9.vs.cso", true);
  Classify(&dev, kOtherLayout, "0x1BB9CBEF.vs.cso", true);
  Classify(&dev, kOpaque, "0x2162672F.ps.cso", false);
  Classify(&dev, kAlpha, "0x049B0385.ps.cso", false);
  Classify(&dev, kNearFadePs, "0x050F5192.ps.cso", false);
  Classify(&dev, kNearFadePs2, "0x06E55EC8.ps.cso", false);
  using contract::kTraitNearFade;
  using contract::kTraitVisibilityLayout;
  for (uint64_t vs : {kRigid, kRigidShadow, kMeshVs, kShadowVs}) {
    const auto traits = contract::LookupVertexTraits(vs);
    CHECK(traits.cls == (uint8_t)contract::VsClass::Rigid && (traits.flags & kTraitVisibilityLayout) != 0u,
          "vs %llx rigid with visibility layout (cls %u flags %u)", (unsigned long long)vs, traits.cls, traits.flags);
    CHECK(contract::LookupVertexClass(vs) == (contract::VsClass)traits.cls, "class lookups agree");
  }
  CHECK((contract::LookupVertexTraits(kOtherLayout).flags & kTraitVisibilityLayout) == 0u, "stride-128 element: no visibility layout");
  {
    using contract::kTraitCameraView;
    using contract::kTraitLightView;
    const uint64_t kShadowVs2 = 0xA7;
    Classify(&dev, kShadowVs2, "0x3168EA98.vs.cso", true);
    for (uint64_t vs : {kRigid, kRigidShadow, kMeshVs}) {
      const uint8_t f = contract::LookupVertexTraits(vs).flags;
      CHECK((f & kTraitCameraView) != 0u && (f & kTraitLightView) == 0u, "vs %llx camera view", (unsigned long long)vs);
    }
    for (uint64_t vs : {kShadowVs, kShadowVs2}) {
      const uint8_t f = contract::LookupVertexTraits(vs).flags;
      CHECK((f & kTraitLightView) != 0u && (f & kTraitCameraView) == 0u, "vs %llx light view", (unsigned long long)vs);
    }
  }
  CHECK((contract::LookupPixelTraits(kOpaque).flags & kTraitNearFade) == 0u, "0x2162672F has no near fade");
  CHECK((contract::LookupPixelTraits(kAlpha).flags & kTraitNearFade) != 0u, "0x049B0385 near-fades");
  CHECK((contract::LookupPixelTraits(kNearFadePs).flags & kTraitNearFade) != 0u, "0x050F5192 near-fades");
  CHECK(contract::LookupPixelClass(kNearFadePs) == contract::PsClass::Opaque, "near fade alone is opaque");
  CHECK((contract::LookupPixelTraits(kNearFadePs2).flags & kTraitNearFade) != 0u, "0x06E55EC8 near-fades");
  {
    const auto none = contract::LookupPixelTraits(0);
    CHECK(none.cls == (uint8_t)contract::PsClass::Opaque && none.flags == 0u, "no PS: opaque, no near fade");
    const auto unknown = contract::LookupPixelTraits(0xDEAD);
    CHECK(unknown.cls == (uint8_t)contract::PsClass::Unclassified, "unknown PS unclassified");
    CHECK(contract::LookupVertexTraits(0).cls == (uint8_t)contract::VsClass::Unclassified, "no VS unclassified");
  }
  {
    const auto entries = contract::SnapshotRegistryEntries();
    int near_fade = 0;
    for (const auto& e : entries.pixel) near_fade += (e.flags & kTraitNearFade) != 0u;
    CHECK(near_fade == 3, "registry entries carry flags (%d)", near_fade);
  }

  // Fade formula.
  {
    float lo, hi;
    CHECK(bvh::CameraFadeInterval(12.f, 0.25f, 0.f, &lo, &hi) && lo == 14.f && hi == bvh::kCameraFadeNever, "interval inv>0 (%f %f)", lo, hi);
    CHECK(bvh::CameraFadeInterval(12.f, 0.25f, 0.5f, &lo, &hi) && lo == 0.f, "floor 0.5 shows everything");
    CHECK(!bvh::CameraFadeInterval(12.f, 0.f, 0.f, &lo, &hi), "inv 0 never shown");
    CHECK(bvh::CameraFadeInterval(12.f, -0.5f, 0.f, &lo, &hi) && lo == 0.f && hi == 11.f, "inv<0 shows near (%f)", hi);
    CHECK(!bvh::CameraFadeInterval(-2.f, -0.5f, 0.f, &lo, &hi), "inv<0 with negative end never shown");
    for (float d : {0.f, 13.9f, 14.f, 14.1f, 20.f}) {
      const bool shown = bvh::CameraNearFade(d, 12.f, 0.25f, 0.f) >= bvh::kCameraFadeShown;
      bvh::CameraFadeInterval(12.f, 0.25f, 0.f, &lo, &hi);
      CHECK(shown == (d >= lo && d <= hi), "formula and interval agree at %f", d);
    }
    CHECK(bvh::CameraNearFade(9.36f, 12.f, 0.25f, 0.f) == 0.f, "inside the start: factor 0 with floor 0");
    CHECK(bvh::CameraNearFade(9.36f, 12.f, 0.25f, 1.f) == 1.f, "floor 1 disables the fade");
    CHECK(!bvh::CameraFadeInputsUsable(NAN, 1.f) && !bvh::CameraFadeInputsUsable(1.f, INFINITY) && bvh::CameraFadeInputsUsable(1.f, 0.f), "usable inputs");
  }

  resource t15, b1, vb1, vb2, vb3, ib;
  const uint64_t ring_elements = 150000;
  dev.create_resource(resource_desc(ring_elements * 160, memory_heap::gpu_only, resource_usage::shader_resource), nullptr, resource_usage::general, &t15);
  const resource_desc cb_desc(16, memory_heap::gpu_only, resource_usage::constant_buffer);
  dev.create_resource(cb_desc, nullptr, resource_usage::general, &b1);
  falcom_world::OnInitResourceCbTracker(&dev, cb_desc, nullptr, resource_usage::general, b1);
  for (resource* r : {&vb1, &vb2, &vb3, &ib}) dev.create_resource(resource_desc(4096, memory_heap::gpu_only, resource_usage::vertex_buffer), nullptr, resource_usage::general, r);
  fixture::WriteVertices(dev.res[vb1.handle].bytes, fixture::Box(1.f));
  fixture::WriteVertices(dev.res[vb2.handle].bytes, fixture::Box(1.f));  // same content: another draw key of mesh 1
  fixture::WriteVertices(dev.res[vb3.handle].bytes, fixture::Box(3.f));
  fixture::WriteIndices(dev.res[ib.handle].bytes);

  falcom_world::WorldCommandListData cl_data;
  cl_data.vs_srv[15] = t15;
  cl_data.vs_cb[1] = b1;

  std::vector<VisObject> objects = {
      // Mesh 1 (vb1): A near-fades in its color pass, B does not; 110 is shadow-only.
      {"A main", vb1.handle, kMeshVs, kNearFadePs, true, {{10, 0, 0}}, {{12.f, 0.25f, 0.f, 1.f}}},
      {"B main", vb1.handle, kMeshVs, kOpaque, true, {{20, 0, 0}}, {{0.f, 1.f, 0.f, 1.f}}},
      {"B2 main", vb1.handle, kMeshVs, kOpaque, true, {{21, 0, 0}}, {{0.f, 1.f, 0.f, 0.25f}}},
      {"A shadow", vb1.handle, kShadowVs, 0, false, {{110, 0, 0}}, {{3.f, 0.5f, 1.f, 1.f}}},
      // A's shadow through another VB: merged at admission, sightings forwarded.
      {"A dup shadow", vb2.handle, kShadowVs, 0, false, {{10, 0, 0}}, {{12.f, 0.25f, 0.f, 1.f}}},
      // Mesh 2 (vb3): every color draw near-fades; 43 is shadow-only.
      {"D main", vb3.handle, kMeshVs, kNearFadePs2, true, {{40, 0, 0}, {41, 0, 0}}, {{5.f, 0.5f, 0.f, 1.f}, {5.f, 0.5f, 0.f, 1.f}}},
      {"D shadow", vb3.handle, kShadowVs, 0, false, {{40, 0, 0}, {41, 0, 0}, {43, 0, 0}}, {{5.f, 0.5f, 0.f, 1.f}, {5.f, 0.5f, 0.f, 1.f}, {7.f, 0.5f, 0.f, 1.f}}},
  };

  uint64_t cursor = 12345;
  float b_param_start = 0.f;  // B's start changes later (changed_since_admission)
  auto run_frame = [&]() {
    for (auto& object : objects) {
      const uint32_t count = static_cast<uint32_t>(object.translations.size());
      if (cursor + count >= ring_elements) cursor = 0;
      const int32_t base = static_cast<int32_t>(cursor);
      for (uint32_t i = 0; i < count; ++i) {
        float t[3] = {object.translations[i][0], object.translations[i][1], object.translations[i][2]};
        float world[12];
        World(t, world);
        uint8_t* element = dev.res[t15.handle].bytes.data() + (cursor + i) * 160;
        std::memcpy(element, world, 48);
        std::memcpy(element + 48, world, 48);
        auto p = object.params[i];
        if (std::string(object.name) == "B main") p[0] = b_param_start;
        const float color[4] = {1.f, 1.f, 1.f, p[3]};
        const float param[4] = {p[0], p[1], p[2], 0.f};
        std::memcpy(element + contract::kColorOffset, color, 16);
        std::memcpy(element + contract::kParamOffset, param, 16);
      }
      cursor += count + 7;
      int32_t cb[4] = {base, 0, 0, 0};
      std::memcpy(dev.res[b1.handle].bytes.data(), cb, 16);
      falcom_world::OnUpdateBufferRegionCbTracker(&dev, cb, b1, 0, UINT64_MAX);
      falcom_world::DrawRecord draw;
      draw.method = 1; draw.has_index_buffer = true; draw.vb = {object.vb}; draw.vb_stride = fixture::kStride; draw.ib = ib;
      draw.vb_size = 4096; draw.ib_size = 4096; draw.input_layout = {fixture::kLayout};
      draw.index_size = 2; draw.dsv = {0x77}; draw.index_count = 36; draw.first_index = 0;
      draw.instance_count = count; draw.depth_enable = true; draw.depth_write = true;
      draw.topology = primitive_topology::triangle_list; draw.vs_pipeline = object.vs; draw.ps_pipeline = object.ps;
      draw.vs_hash = static_cast<uint32_t>(object.vs); draw.ps_hash = static_cast<uint32_t>(object.ps);
      draw.rtv0 = {0x88u};  // like the game: a render target is bound in shadow passes too
      bvh::OnPoolScanDraw(&dev, &cl, draw, &cl_data);
    }
    falcom_world::g_state.frame.fetch_add(1u);
    bvh::DrainPoolScan(&dev, &queue);
  };

  bvh::g_pool.scan_active.store(true);
  for (int f = 0; f < 70; ++f) run_frame();
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    auto& p = bvh::g_pool;
    std::printf("visibility: meshes %zu instances %zu\n", p.meshes.size(), p.instances.size());
    CHECK(p.meshes.size() == 2 && p.instances.size() == 7, "meshes %zu instances %zu", p.meshes.size(), p.instances.size());
    const auto* a = FindInstance(10.f);
    const auto* b = FindInstance(20.f);
    const auto* b2 = FindInstance(21.f);
    const auto* s110 = FindInstance(110.f);
    const auto* d40 = FindInstance(40.f);
    const auto* s43 = FindInstance(43.f);
    CHECK(a && b && b2 && s110 && d40 && s43, "instances present");
    if (a && b && b2 && s110 && d40 && s43) {
      const auto va = bvh::GetPoolCameraVisibility(*a);
      std::printf("A: source %s near %d param %.2f %.2f camera %u light %u ps %08X\n", va.source, va.near_fade,
                  va.inputs.param[0], va.inputs.param[1], va.camera_frames, va.light_frames, va.camera_ps_hash);
      CHECK(va.inputs.valid && std::string(va.source) == "camera draw" && va.inputs.param[0] == 12.f && va.inputs.param[1] == 0.25f,
            "A inputs from the camera draw");
      CHECK(va.near_fade && va.camera_seen && va.camera_ps_hash == (uint32_t)kNearFadePs, "A near-fades (own draws)");
      CHECK(va.camera_frames > 0 && va.light_frames > 0 && va.light_seen && va.observed, "A seen in both views (forwarded across draw keys)");
      CHECK(p.stats.dedup_instances >= 1u, "A merged across draw keys (%u)", p.stats.dedup_instances);
      CHECK(a->visibility.valid && a->visibility.param[0] == 12.f, "A admission inputs");
      const auto vb = bvh::GetPoolCameraVisibility(*b);
      CHECK(!vb.near_fade && vb.camera_seen && !vb.light_seen, "B does not near-fade, camera only");
      const auto vb2 = bvh::GetPoolCameraVisibility(*b2);
      CHECK(vb2.inputs.color[3] == 0.25f, "B2 opacity read");
      const auto vs = bvh::GetPoolCameraVisibility(*s110);
      std::printf("S110: source %s near %d camera %u light %u\n", vs.source, vs.near_fade, vs.camera_frames, vs.light_frames);
      CHECK(std::string(vs.source) == "light draw" && !vs.camera_seen && vs.light_seen && !vs.near_fade
                && vs.camera_frames == 0u && vs.light_frames > 0u && vs.inputs.param[0] == 3.f && vs.inputs.param[2] == 1.f,
            "shadow-only instance");
      const auto& mesh1 = p.meshes[a->mesh_id];
      std::printf("mesh1: camera %u near %u light %u\n", mesh1.camera_draws, mesh1.camera_near_fade_draws, mesh1.light_draws);
      CHECK(mesh1.camera_draws > 0 && mesh1.camera_near_fade_draws > 0 && mesh1.camera_near_fade_draws * 2 < mesh1.camera_draws
                && mesh1.light_draws > 0, "mesh 1 view counts");
      const auto vd = bvh::GetPoolCameraVisibility(*d40);
      CHECK(vd.near_fade && vd.camera_seen && vd.light_seen, "D near-fades, both views");
      const auto v43 = bvh::GetPoolCameraVisibility(*s43);
      CHECK(!v43.camera_seen && v43.light_seen && !v43.near_fade && v43.inputs.param[0] == 7.f, "shadow-only instance of a near-fading mesh");
    }
  }

  // Later sightings refresh the inputs (B's start moves to 2 m) and ask for a
  // TLAS rebuild once; unchanged sightings do not.
  uint64_t visibility_before = 0u;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    bvh::g_pool.schedule.clear();
  }
  for (int f = 0; f < 3; ++f) run_frame();
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    visibility_before = bvh::g_pool.visibility_revision;
    bvh::g_pool.schedule.clear();
  }
  for (int f = 0; f < 3; ++f) run_frame();
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    CHECK(bvh::g_pool.visibility_revision == visibility_before, "steady sightings change nothing (%llu -> %llu)",
          (unsigned long long)visibility_before, (unsigned long long)bvh::g_pool.visibility_revision);
  }
  b_param_start = 2.f;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    bvh::g_pool.schedule.clear();
  }
  for (int f = 0; f < 3; ++f) run_frame();
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    const auto* b = FindInstance(20.f);
    CHECK(b != nullptr, "B");
    if (b) {
      const auto vb = bvh::GetPoolCameraVisibility(*b);
      CHECK(vb.inputs.param[0] == 2.f && b->visibility.param[0] == 0.f, "latest vs admission (%f %f)", vb.inputs.param[0], b->visibility.param[0]);
    }
    CHECK(bvh::g_pool.visibility_revision == visibility_before + 1u, "one visibility change for B (%llu -> %llu)",
          (unsigned long long)visibility_before, (unsigned long long)bvh::g_pool.visibility_revision);
  }

  // Dump: scene constants and the visibility section.
  {
    std::lock_guard lock(falcom_world::g_state.mutex);
    falcom_world::g_state.camera.valid = true;
    falcom_world::g_state.camera.view_inv[3] = 1.f;
    falcom_world::g_state.camera.view_inv[7] = 0.f;
    falcom_world::g_state.camera.view_inv[11] = 0.f;
    falcom_world::g_state.camera.has_fade = true;
    falcom_world::g_state.camera.near_fade_floor = 0.f;
    falcom_world::g_state.camera.map_alpha = 1.f;
  }
  std::filesystem::remove(bvh::PoolOutputDir() / "world_pool.json");
  bvh::DumpWorldPool();
  {
    std::ifstream f(bvh::PoolOutputDir() / "world_pool.json");
    const std::string text((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    CHECK(text.find("\"schema\": 9") != std::string::npos, "schema 9");
    CHECK(text.find("\"scene\": {\"fade_constants\": true") != std::string::npos, "scene constants");
    CHECK(text.find("\"visibility\": {\"instances\": 7, \"camera_visible\": 5, \"shadow_only\": 2") != std::string::npos
              && text.find("\"with_inputs\": 7") != std::string::npos, "visibility summary");
    CHECK(text.find("\"view\": \"light\"") != std::string::npos && text.find("\"view\": \"camera\"") != std::string::npos, "VS views in the dump");
    CHECK(text.find("\"visibility_changes\": ") != std::string::npos && text.find("nan") == std::string::npos, "visibility changes, no nan");
    CHECK(text.find("\"near_fade\": true") != std::string::npos, "pixel shader near_fade flag");
    CHECK(text.find("\"near_fade_draws\"") != std::string::npos, "mesh near-fade draws");
    CHECK(text.find("\"changed_since_admission\": 1") != std::string::npos, "changed since admission");
    CHECK(text.find("\"hidden_at_camera\": 1") != std::string::npos, "A hidden from a camera 8 m away");
    CHECK(text.find("\"reason\": \"shadow-only\"") != std::string::npos && text.find("\"reason\": \"near fade\"") != std::string::npos, "samples");
    const size_t at = text.find("\"visibility\"");
    std::printf("%s\n", text.substr(at, 900).c_str());
  }

  bvh::OnDestroyDevicePool(&dev);
  std::printf("%s (%d failures)\n", g_failures == 0 ? "PASS" : "FAILED", g_failures);
  return g_failures == 0 ? 0 : 1;
}
