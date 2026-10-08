// Native harness: deforming meshes in the BVH (path 1, S2). The mock command
// list plays the stream-output stage (position-only captures of an animated
// grid), the test runs a transcription of world_bvh_refit and of the dynamic
// part of world_bvh_trace over the uploaded buffers and compares with brute
// force over this frame's triangles.
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <random>
#include <set>
#include <span>
#include "gen/mock_base.hpp"
#include "harness_timer.hpp"
#define __world_bvh_refit_EMBED_FILE
inline constexpr std::uint8_t __world_bvh_refit_base[] = {0};
inline constexpr std::span<const std::uint8_t> __world_bvh_refit{__world_bvh_refit_base};
#include "src/games/falcomengine-plus/world/bvh/deform_live.hpp"

using namespace reshade::api;
namespace bvh = falcom_world::bvh;
namespace contract = falcom_world::contract;

static int g_failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++g_failures; std::printf("FAIL %s:%d %s | ", __FILE__, __LINE__, #cond); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static int g_under_lock = 0;
static void ProbeLock() {
  if (bvh::g_deform_live.mutex.try_lock()) bvh::g_deform_live.mutex.unlock();
  else ++g_under_lock;
}

struct Res { resource_desc desc; std::vector<uint8_t> bytes; };
struct Device : mock::DeviceBase {
  std::map<uint64_t, Res> res;
  std::map<uint64_t, uint64_t> views;
  uint64_t next = 0x1000, next_view = 0x900000, next_object = 0x700000;
  int alive_objects = 0;
  device_api get_api() const override { return device_api::d3d11; }
  bool create_resource(const resource_desc &desc, const subresource_data *initial_data, resource_usage, resource *out, void ** = nullptr) override {
    ProbeLock();
    Res r; r.desc = desc; r.bytes.assign(desc.buffer.size, 0xCD);
    if (initial_data && initial_data->data) std::memcpy(r.bytes.data(), initial_data->data, desc.buffer.size);
    out->handle = next; next += 0x100; res[out->handle] = std::move(r);
    return true;
  }
  void destroy_resource(resource r) override { ProbeLock(); res.erase(r.handle); }
  resource_desc get_resource_desc(resource r) const override {
    auto it = res.find(r.handle);
    if (it == res.end()) return resource_desc{};
    resource_desc desc = it->second.desc;
    if (desc.type == resource_type::buffer) desc.buffer.stride = 0u;
    return desc;
  }
  bool create_resource_view(resource r, resource_usage, const resource_view_desc &, resource_view *out) override {
    ProbeLock();
    if (res.count(r.handle) == 0u) return false;
    out->handle = next_view++;
    views[out->handle] = r.handle;
    return true;
  }
  void destroy_resource_view(resource_view v) override { ProbeLock(); views.erase(v.handle); }
  bool map_buffer_region(resource r, uint64_t offset, uint64_t size, map_access, void **out) override {
    auto it = res.find(r.handle); if (it == res.end() || offset + size > it->second.bytes.size()) return false;
    *out = it->second.bytes.data() + offset; return true;
  }
  void unmap_buffer_region(resource) override {}
  void update_buffer_region(const void *data, resource dest, uint64_t dest_offset, uint64_t size) override {
    ProbeLock();
    auto it = res.find(dest.handle);
    if (it == res.end()) return;
    if (dest_offset == 0u) size = it->second.bytes.size();  // ReShade D3D11 quirk (see WriteBufferRange)
    if (dest_offset + size > it->second.bytes.size()) return;
    std::memcpy(it->second.bytes.data() + dest_offset, data, size);
  }
  bool create_pipeline_layout(uint32_t, const pipeline_layout_param *, pipeline_layout *out) override { out->handle = next_object++; ++alive_objects; return true; }
  void destroy_pipeline_layout(pipeline_layout) override { --alive_objects; }
  bool allocate_descriptor_tables(uint32_t count, pipeline_layout, uint32_t, descriptor_table *out) override {
    for (uint32_t i = 0; i < count; ++i) { out[i].handle = next_object++; ++alive_objects; }
    return true;
  }
  void free_descriptor_tables(uint32_t count, const descriptor_table *) override { alive_objects -= static_cast<int>(count); }
  bool create_pipeline(pipeline_layout, uint32_t, const pipeline_subobject *, pipeline *out) override { out->handle = next_object++; ++alive_objects; return true; }
  void destroy_pipeline(pipeline) override { --alive_objects; }
  void update_descriptor_tables(uint32_t, const descriptor_table_update *) override { ProbeLock(); }
  const Res* ViewRes(resource_view v) const {
    auto it = views.find(v.handle); if (it == views.end()) return nullptr;
    auto r = res.find(it->second); return r == res.end() ? nullptr : &r->second;
  }
};

// An animated grid: vertex k of triangle t of a mesh (vb handle) at frame f.
static void PoseVertex(uint64_t mesh, uint32_t tri, uint32_t k, uint32_t frame, float out[3]) {
  const uint32_t cell = tri / 2u, row = cell / 10u, col = cell % 10u;
  static const int corner[2][3][2] = {{{0, 0}, {0, 1}, {1, 0}}, {{1, 0}, {0, 1}, {1, 1}}};
  const int* c = corner[tri % 2u][k];
  const float x = static_cast<float>(col + c[0]) * 0.2f + static_cast<float>(mesh % 7u) * 5.f;
  const float z = static_cast<float>(row + c[1]) * 0.2f;
  out[0] = x + 0.05f * std::sin(static_cast<float>(frame) * 0.3f + z);
  out[1] = 1.f + 0.3f * std::sin(static_cast<float>(frame) * 0.2f + x * 2.f);
  out[2] = z - 20.f;
}

struct CmdList : mock::CommandListBase {
  Device* dev = nullptr;
  uint64_t gs = 0u, mesh = 0u;
  resource so = {0u};
  uint64_t so_offset = 0u;
  std::vector<std::string> calls;
  int dispatches = 0;
  device *get_device() override { return dev; }
  void bind_pipeline(pipeline_stage stages, pipeline p) override {
    ProbeLock();
    if (stages == (pipeline_stage::geometry_shader | pipeline_stage::stream_output)) { gs = p.handle; calls.push_back(p.handle ? "gs" : "gs0"); }
  }
  void bind_stream_output_buffers(uint32_t, uint32_t count, const resource *buffers, const uint64_t *offsets, const uint64_t *, const resource *, const uint64_t *) override {
    ProbeLock();
    if (count == 1u) { so = buffers[0]; so_offset = offsets[0]; }
    calls.push_back(so.handle ? "so" : "so0");
  }
  void draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t, int32_t, uint32_t) override {
    ProbeLock();
    calls.push_back("draw");
    if (gs == 0u || so.handle == 0u) return;
    auto& bytes = dev->res[so.handle].bytes;
    const uint32_t triangles = index_count / 3u * instance_count;
    const uint32_t frame = falcom_world::g_state.frame.load();
    for (uint32_t t = 0; t < triangles; ++t) {
      for (uint32_t k = 0; k < 3u; ++k) {
        float p[3];
        PoseVertex(mesh, t, k, frame, p);
        const uint64_t at = so_offset + (uint64_t(t) * 3u + k) * 12u;
        if (at + 12u > bytes.size()) return;
        std::memcpy(bytes.data() + at, p, 12u);
      }
    }
  }
  void copy_buffer_region(resource src, uint64_t so_, resource dst, uint64_t dof, uint64_t size) override {
    ProbeLock();
    auto s = dev->res.find(src.handle), d = dev->res.find(dst.handle);
    if (s == dev->res.end() || d == dev->res.end() || so_ + size > s->second.bytes.size() || dof + size > d->second.bytes.size()) return;
    std::memcpy(d->second.bytes.data() + dof, s->second.bytes.data() + so_, size);
  }
  void dispatch(uint32_t, uint32_t, uint32_t) override { ProbeLock(); ++dispatches; }
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
// What the probe records once it matched a shader's world position.
static void Confirm(uint64_t pipeline, const char* semantic, uint32_t index) {
  const auto code = contract::LookupDeformingCode(pipeline);
  std::lock_guard lock(bvh::g_deform.mutex);
  bvh::DeformShader& shader = bvh::DeformShaderEntry(pipeline, *code, code);
  for (size_t e = 0; e < shader.layout.elements.size(); ++e) {
    if (shader.layout.elements[e].semantic == semantic && shader.layout.elements[e].semantic_index == index) shader.chosen = static_cast<int32_t>(e);
  }
  shader.matched = 1u;
}

static int g_created = 0, g_destroyed = 0;
static std::set<uint64_t> g_alive_gs;
static std::vector<bvh::DeformSoLayout> g_layouts;
static bvh::DeformSkip g_game_state = bvh::DeformSkip::None;
static uint64_t MockCreate(device*, const contract::ShaderCode&, const bvh::DeformSoLayout& layout, int32_t* hr) {
  *hr = 0; g_layouts.push_back(layout); const uint64_t h = 0xC0DE0000u + ++g_created; g_alive_gs.insert(h); return h;
}
static void MockDestroy(uint64_t s) { ++g_destroyed; g_alive_gs.erase(s); }
static bvh::DeformSkip MockCheck(command_list*) { return g_game_state; }

// world_bvh_refit, transcribed (one "group" per object, levels deepest first).
static void EmulateRefit(Device& dev) {
  auto& st = bvh::g_deform_live;
  const auto* verts = reinterpret_cast<const float*>(dev.res[st.arena.handle].bytes.data());
  const auto* objects = reinterpret_cast<const bvh::DynamicObjectGPU*>(dev.res[st.objects_buffer.handle].bytes.data());
  auto* nodes = reinterpret_cast<bvh::BVHNodeGPU*>(dev.res[st.nodes.buffer.handle].bytes.data());
  const auto* leaves = reinterpret_cast<const bvh::BVHLeafGPU*>(dev.res[st.leaves.buffer.handle].bytes.data());
  const auto* refit = reinterpret_cast<const uint32_t*>(dev.res[st.refit.buffer.handle].bytes.data());
  for (uint32_t o = 0; o < st.object_count; ++o) {
    const bvh::DynamicObjectGPU obj = objects[o];
    const uint32_t leaf_count = (obj.node_count + 1u) / 2u;
    for (uint32_t level = 0; level < 64u; ++level) {
      if (level >= obj.level_count) continue;
      const uint32_t begin = refit[obj.refit_offset + obj.level_offset + level];
      const uint32_t end = refit[obj.refit_offset + obj.level_offset + level + 1u];
      for (uint32_t e = begin; e < end; ++e) {
        const uint32_t local = refit[obj.refit_offset + e];
        if (local >= obj.node_count) continue;
        bvh::BVHNodeGPU& node = nodes[obj.node_offset + local];
        float lo[3], hi[3];
        if (node.child_or_leaf & bvh::kBvhLeafFlag) {
          const uint32_t leaf = node.child_or_leaf & 0x7FFFFFFFu;
          if (leaf >= leaf_count) continue;
          const uint32_t prim = leaves[obj.leaf_offset + leaf].prim;
          if (prim >= obj.triangle_count) continue;
          const float* a = verts + (size_t(obj.vertex_base) + prim * 3u) * 3u;
          for (int k = 0; k < 3; ++k) { lo[k] = std::min({a[k], a[3 + k], a[6 + k]}); hi[k] = std::max({a[k], a[3 + k], a[6 + k]}); }
        } else {
          const bvh::BVHNodeGPU& l = nodes[obj.node_offset + node.child_or_leaf];
          const bvh::BVHNodeGPU& r = nodes[obj.node_offset + node.sibling_or_right];
          for (int k = 0; k < 3; ++k) { lo[k] = std::min(l.bounds_min[k], r.bounds_min[k]); hi[k] = std::max(l.bounds_max[k], r.bounds_max[k]); }
        }
        for (int k = 0; k < 3; ++k) { node.bounds_min[k] = lo[k]; node.bounds_max[k] = hi[k]; }
      }
    }
  }
}

// Ray/triangle as in world_bvh_trace.hlsli (Moller-Trumbore).
static bool RayTri(const float o[3], const float d[3], const float* v0, const float* v1, const float* v2, float* t) {
  const float e1[3] = {v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2]}, e2[3] = {v2[0] - v0[0], v2[1] - v0[1], v2[2] - v0[2]};
  const float p[3] = {d[1] * e2[2] - d[2] * e2[1], d[2] * e2[0] - d[0] * e2[2], d[0] * e2[1] - d[1] * e2[0]};
  const float det = e1[0] * p[0] + e1[1] * p[1] + e1[2] * p[2];
  if (std::fabs(det) < 1e-12f) return false;
  const float inv = 1.f / det;
  const float tv[3] = {o[0] - v0[0], o[1] - v0[1], o[2] - v0[2]};
  const float u = (tv[0] * p[0] + tv[1] * p[1] + tv[2] * p[2]) * inv;
  if (u < 0.f || u > 1.f) return false;
  const float q[3] = {tv[1] * e1[2] - tv[2] * e1[1], tv[2] * e1[0] - tv[0] * e1[2], tv[0] * e1[1] - tv[1] * e1[0]};
  const float v = (d[0] * q[0] + d[1] * q[1] + d[2] * q[2]) * inv;
  if (v < 0.f || u + v > 1.f) return false;
  const float ht = (e2[0] * q[0] + e2[1] * q[1] + e2[2] * q[2]) * inv;
  if (ht <= 0.f) return false;
  *t = ht;
  return true;
}
static bool RayBox(const float o[3], const float d[3], const float* lo, const float* hi, float tmin, float tmax) {
  float tn = tmin, tf = tmax;
  for (int k = 0; k < 3; ++k) {
    const float inv = 1.f / d[k];
    float a = (lo[k] - o[k]) * inv, b = (hi[k] - o[k]) * inv;
    if (a > b) std::swap(a, b);
    tn = std::max(tn, a); tf = std::min(tf, b);
  }
  return tn <= tf;
}

// The dynamic loop of TraceWorldRay + TraceDynamicBlas, transcribed.
static float EmulateTraceDynamic(Device& dev, const float o[3], const float d[3], uint32_t* hit_object, uint32_t* hit_prim) {
  auto& st = bvh::g_deform_live;
  const auto* verts = reinterpret_cast<const float*>(dev.res[st.arena.handle].bytes.data());
  const auto* objects = reinterpret_cast<const bvh::DynamicObjectGPU*>(dev.res[st.objects_buffer.handle].bytes.data());
  const auto* nodes = reinterpret_cast<const bvh::BVHNodeGPU*>(dev.res[st.nodes.buffer.handle].bytes.data());
  const auto* leaves = reinterpret_cast<const bvh::BVHLeafGPU*>(dev.res[st.leaves.buffer.handle].bytes.data());
  float best = 1e30f;
  for (uint32_t i = 0; i < st.object_count; ++i) {
    const bvh::DynamicObjectGPU obj = objects[i];
    const bvh::BVHNodeGPU& root = nodes[obj.node_offset];
    if (!RayBox(o, d, root.bounds_min, root.bounds_max, 0.001f, best)) continue;
    std::vector<uint32_t> stack = {0u};
    while (!stack.empty()) {
      const uint32_t n = stack.back(); stack.pop_back();
      const bvh::BVHNodeGPU& node = nodes[obj.node_offset + n];
      if (!RayBox(o, d, node.bounds_min, node.bounds_max, 0.001f, best)) continue;
      if (node.child_or_leaf & bvh::kBvhLeafFlag) {
        const uint32_t prim = leaves[obj.leaf_offset + (node.child_or_leaf & 0x7FFFFFFFu)].prim;
        const float* a = verts + (size_t(obj.vertex_base) + prim * 3u) * 3u;
        float t;
        if (RayTri(o, d, a, a + 3, a + 6, &t) && t >= 0.001f && t < best) { best = t; *hit_object = i; *hit_prim = prim; }
      } else {
        stack.push_back(node.sibling_or_right);
        stack.push_back(node.child_or_leaf);
      }
    }
  }
  return best;
}

int main() {
  Device dev; CmdList cl; cl.dev = &dev; Queue queue; queue.dev = &dev; queue.cl = &cl;
  bvh::g_deform.backend = {MockCreate, MockDestroy, MockCheck, nullptr, nullptr, nullptr, nullptr, nullptr};
  bvh::g_pool.immediate_cmd_list.store(reinterpret_cast<uint64_t>(static_cast<command_list*>(&cl)));
  const uint64_t kSkin = 0x51A, kSkinB = 0x51B, kUnconfirmed = 0x51C, kLight = 0x51D;
  Classify(&dev, kSkin, "0x51654E2F.vs.cso");
  Classify(&dev, kSkinB, "0x4A037EB5.vs.cso");
  Classify(&dev, kUnconfirmed, "0x37E8915A.vs.cso");
  Classify(&dev, kLight, "0x0417B98E.vs.cso");
  Confirm(kSkin, "TEXCOORD", 1);
  Confirm(kSkinB, "TEXCOORD", 1);

  HStage("0. Refit order: every node once, children before parents.");
  // 0. Refit order: every node once, children before parents.
  {
    std::mt19937 rng(7);
    for (int trial = 0; trial < 50; ++trial) {
      std::vector<std::array<float, 3>> positions;
      std::vector<uint32_t> indices;
      const int tris = 1 + static_cast<int>(rng() % 400);
      for (int t = 0; t < tris * 3; ++t) {
        positions.push_back({float(rng() % 1000) * 0.01f, float(rng() % 1000) * 0.01f, float(rng() % 1000) * 0.01f});
        indices.push_back(static_cast<uint32_t>(t));
      }
      bvh::MeshBlas blas;
      if (bvh::BuildMeshBlas(positions, indices, &blas) != nullptr) continue;
      std::vector<uint32_t> order, starts;
      CHECK(bvh::BuildRefitOrder(blas.nodes, &order, &starts), "order built");
      std::vector<int> position(blas.nodes.size(), -1);
      for (size_t e = 0; e < order.size(); ++e) position[order[e]] = static_cast<int>(e);
      bool ok = order.size() == blas.nodes.size() && starts.back() == order.size();
      for (size_t n = 0; n < blas.nodes.size() && ok; ++n) {
        ok = position[n] >= 0;
        const auto& node = blas.nodes[n];
        if (ok && !(node.child_or_leaf & bvh::kBvhLeafFlag)) ok = position[node.child_or_leaf] < position[n] && position[node.sibling_or_right] < position[n];
      }
      CHECK(ok, "order trial %d", trial);
      // Moved positions: the refit equals a fresh bottom-up fit of the same tree.
      for (auto& p : positions) p[1] += 0.5f * std::sin(p[0]);
      std::vector<float> flat;
      for (auto& p : positions) flat.insert(flat.end(), p.begin(), p.end());
      auto refit = blas.nodes;
      bvh::RefitDynamicNodes(&refit, blas.leaves, order, starts, flat.data(), static_cast<uint32_t>(tris));
      std::vector<bvh::BVHLeafGPU> moved = blas.leaves;
      for (auto& leaf : moved) {
        const float* a = flat.data() + leaf.prim * 9u;
        for (int k = 0; k < 3; ++k) { leaf.bounds_min[k] = std::min({a[k], a[3 + k], a[6 + k]}); leaf.bounds_max[k] = std::max({a[k], a[3 + k], a[6 + k]}); }
      }
      CHECK(bvh::ValidateBvh(refit, moved).ok, "refit tree valid (trial %d)", trial);
    }
  }

  auto draw_for = [&](uint64_t vs, uint64_t mesh, uint32_t index_count, uint32_t first_index = 0u) {
    falcom_world::DrawRecord draw;
    draw.method = 1; draw.has_index_buffer = true; draw.vs_pipeline = vs; draw.index_count = index_count;
    draw.instance_count = 1; draw.topology = primitive_topology::triangle_list; draw.vb = {mesh}; draw.ib = {0x77};
    draw.first_index = first_index; draw.frame = falcom_world::g_state.frame.load();
    return draw;
  };
  auto draw = [&](uint64_t vs, uint64_t mesh, uint32_t index_count, uint32_t first_index = 0u) {
    cl.mesh = mesh;
    bvh::OnDeformCaptureDraw(&dev, &cl, draw_for(vs, mesh, index_count, first_index));
  };
  auto present = [&]() {
    bvh::UpdateDeformLive(&dev, &queue);
    falcom_world::g_state.frame.fetch_add(1u);
  };
  auto stats = [&]() { std::lock_guard lock(bvh::g_deform_live.mutex); return bvh::g_deform_live.stats; };

  HStage("1. Off: nothing.");
  // 1. Off: nothing.
  bvh::g_deform_live.enabled.store(false);
  draw(kSkin, 0x11, 600);
  present();
  CHECK(cl.calls.empty() && dev.res.empty(), "off");

  HStage("2. On: resources, then the capture shader, then captures; the tree after the topology read.");
  // 2. On: resources, then the capture shader, then captures; the tree after the topology read.
  bvh::g_deform_live.enabled.store(true);
  present();
  CHECK(dev.res.size() == 5u, "arena + 3 staging + objects (%zu)", dev.res.size());
  draw(kSkin, 0x11, 600);
  CHECK(g_created == 1 && cl.calls.empty(), "first draw builds the capture shader");
  CHECK(g_layouts.back().stride == 12u && g_layouts.back().elements.size() == 1u && g_layouts.back().elements[0].semantic == "TEXCOORD"
            && g_layouts.back().elements[0].semantic_index == 1u && g_layouts.back().elements[0].count == 3u, "position-only layout");
  present();
  draw(kSkin, 0x11, 600);
  CHECK((cl.calls == std::vector<std::string>{"gs", "so", "draw", "so0", "gs0"}), "bind/draw/unbind");
  CHECK(cl.gs == 0u && cl.so.handle == 0u, "state restored");
  present();
  CHECK(stats().frame_captures == 1u && stats().frame_pending == 1u && bvh::g_deform_live.object_count == 0u, "pending");
  for (int i = 0; i < 4 && bvh::g_deform_live.object_count == 0u; ++i) { draw(kSkin, 0x11, 600); present(); }
  {
    const auto s = stats();
    std::printf("after topology: identities %u resident %u built %u objects %u\n", s.identities, s.resident, s.blas_built, bvh::g_deform_live.object_count);
    CHECK(s.blas_built == 1u && s.resident == 1u && bvh::g_deform_live.object_count == 1u, "traced");
  }

  HStage("3. Refit over this frame's pose and trace vs brute force.");
  // 3. Refit over this frame's pose and trace vs brute force.
  auto check_frame = [&](const char* label) {
    EmulateRefit(dev);
    auto& st = bvh::g_deform_live;
    const auto* objects = reinterpret_cast<const bvh::DynamicObjectGPU*>(dev.res[st.objects_buffer.handle].bytes.data());
    const auto* verts = reinterpret_cast<const float*>(dev.res[st.arena.handle].bytes.data());
    const auto* nodes = reinterpret_cast<const bvh::BVHNodeGPU*>(dev.res[st.nodes.buffer.handle].bytes.data());
    for (uint32_t i = 0; i < st.object_count; ++i) {
      const auto obj = objects[i];
      float lo[3] = {1e30f, 1e30f, 1e30f}, hi[3] = {-1e30f, -1e30f, -1e30f};
      for (uint32_t v = 0; v < obj.triangle_count * 3u; ++v)
        for (int k = 0; k < 3; ++k) { lo[k] = std::min(lo[k], verts[(obj.vertex_base + v) * 3u + k]); hi[k] = std::max(hi[k], verts[(obj.vertex_base + v) * 3u + k]); }
      bool same = true;
      for (int k = 0; k < 3; ++k) same = same && nodes[obj.node_offset].bounds_min[k] == lo[k] && nodes[obj.node_offset].bounds_max[k] == hi[k];
      CHECK(same, "%s: root bounds = this frame's triangles (object %u)", label, i);
    }
    std::mt19937 rng(11);
    int mismatches = 0, hits = 0;
    for (int r = 0; r < 400; ++r) {
      // Aim at one object's root bounds (grown by 0.3 m so some rays miss).
      const auto aim = nodes[objects[uint32_t(r) % st.object_count].node_offset];
      const float ux = float(rng() % 1000) / 1000.f, uz = float(rng() % 1000) / 1000.f;
      const float o[3] = {aim.bounds_min[0] - 0.3f + ux * (aim.bounds_max[0] - aim.bounds_min[0] + 0.6f), 6.f,
                          aim.bounds_min[2] - 0.3f + uz * (aim.bounds_max[2] - aim.bounds_min[2] + 0.6f)};
      const float d[3] = {0.f, -1.f, 0.f};
      uint32_t ho = UINT32_MAX, hp = UINT32_MAX;
      const float t = EmulateTraceDynamic(dev, o, d, &ho, &hp);
      float best = 1e30f;
      for (uint32_t i = 0; i < st.object_count; ++i) {
        const auto obj = objects[i];
        for (uint32_t p = 0; p < obj.triangle_count; ++p) {
          const float* a = verts + (size_t(obj.vertex_base) + p * 3u) * 3u;
          float tt;
          if (RayTri(o, d, a, a + 3, a + 6, &tt) && tt >= 0.001f && tt < best) best = tt;
        }
      }
      if (best < 1e29f) ++hits;
      if (best != t) ++mismatches;
    }
    std::printf("%s: %d hits of 400, mismatches %d\n", label, hits, mismatches);
    CHECK(mismatches == 0 && hits > 200 && hits < 400, "%s: trace = brute force", label);
  };
  check_frame("one object");

  HStage("4. A second identity and a duplicate; unconfirmed, light view, deferred, game state.");
  // 4. A second identity and a duplicate; unconfirmed, light view, deferred, game state.
  draw(kSkin, 0x11, 600);
  draw(kSkin, 0x11, 600);              // duplicate
  draw(kSkinB, 0x12, 300);             // builds its shader
  draw(kSkinB, 0x12, 300);
  draw(kUnconfirmed, 0x13, 300);
  draw(kLight, 0x14, 300);
  CmdList deferred; deferred.dev = &dev;
  bvh::OnDeformCaptureDraw(&dev, &deferred, draw_for(kSkin, 0x15, 300));
  g_game_state = bvh::DeformSkip::GameGeometryShader;
  cl.calls.clear();
  draw(kSkin, 0x16, 300);
  g_game_state = bvh::DeformSkip::None;
  CHECK(cl.calls.empty() && deferred.calls.empty(), "no binds for skipped draws");
  {
    const auto s = stats();
    CHECK(s.skips[size_t(bvh::DynamicSkip::Duplicate)] == 1u && s.skips[size_t(bvh::DynamicSkip::NotConfirmed)] == 1u
              && s.skips[size_t(bvh::DynamicSkip::LightView)] == 1u && s.skips[size_t(bvh::DynamicSkip::Deferred)] == 1u
              && s.skips[size_t(bvh::DynamicSkip::GameState)] == 1u, "skip reasons");
  }
  present();
  for (int i = 0; i < 5; ++i) { draw(kSkin, 0x11, 600); draw(kSkinB, 0x12, 300); present(); }
  {
    const auto s = stats();
    CHECK(bvh::g_deform_live.object_count == 2u && s.resident == 2u, "two objects (%u, resident %u)", bvh::g_deform_live.object_count, s.resident);
    const auto* objects = reinterpret_cast<const bvh::DynamicObjectGPU*>(dev.res[bvh::g_deform_live.objects_buffer.handle].bytes.data());
    CHECK(objects[0].vertex_base == 0u && objects[1].vertex_base == 600u && objects[1].triangle_count == 100u, "vertex bases follow capture order");
  }
  check_frame("two objects");

  HStage("5. Arena full; inspect.");
  // 5. Arena full; inspect.
  draw(kSkin, 0x17, 3u * 800000u);
  CHECK(stats().skips[size_t(bvh::DynamicSkip::ArenaFull)] == 1u, "arena full");
  {
    bvh::BvhInspect inspect;
    inspect.valid = inspect.hit = true;
    inspect.instance = bvh::kDynamicInstanceFlag | 1u;
    inspect.prim = 5u;
    bvh::DescribeDynamicInspect(inspect, "match");
    for (auto& line : inspect.lines) std::printf("  %s\n", line.c_str());
    CHECK(inspect.lines.size() == 3u && inspect.lines[1].find("VS 0x4A037EB5") != std::string::npos, "inspect");
  }
  present();

  HStage("6. An identity no longer drawn retires; its bytes become garbage.");
  // 6. An identity no longer drawn retires; its bytes become garbage.
  for (int i = 0; i < 610; ++i) { draw(kSkin, 0x11, 600); present(); }
  {
    const auto s = stats();
    CHECK(s.retired == 1u && s.identities == 1u && s.garbage_bytes > 0u, "retired (%u, ids %u)", s.retired, s.identities);
  }
  draw(kSkin, 0x11, 600);
  present();
  check_frame("after retirement");

  HStage("8. World deform presence dump: identities with gaps, the stats and the dropout ring.");
  // 8. World deform presence dump: identities with gaps, the stats and the dropout ring.
  bvh::DumpDeformLive();
  {
    size_t identities = 0u;
    {
      std::lock_guard<std::mutex> lock(bvh::g_deform_live.mutex);
      identities = bvh::g_deform_live.identities.size();
    }
    CHECK(identities > 0u, "identities exist for the dump (%zu)", identities);
    std::ifstream f(bvh::PoolOutputDir() / "world_deform_live.json");
    const std::string text((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    CHECK(text.find("\"schema\": 1") != std::string::npos && text.find("\"presents_no_objects\": ") != std::string::npos
              && text.find("\"new_after_warmup\": ") != std::string::npos && text.find("\"dropouts\": [") != std::string::npos,
          "deform presence dump has schema, stats and dropouts");
    CHECK(text.find("\"identities\": [\n    {\"key\": ") != std::string::npos && text.find("\"gaps\": ") != std::string::npos
              && text.find("\"max_gap\": ") != std::string::npos && text.find("\"pending_frames\": ") != std::string::npos,
          "deform presence dump: identities with gaps, max_gap and pending_frames");
  }

  HStage("7. Shader destroyed; off releases everything.");
  // 7. Shader destroyed; off releases everything.
  bvh::OnDestroyPipelineDeformLive(&dev, pipeline{kSkinB});
  bvh::g_deform_live.enabled.store(false);
  present();
  CHECK(dev.res.empty() && dev.views.empty() && dev.alive_objects == 0, "released (%zu res, %zu views, %d objects)", dev.res.size(), dev.views.size(), dev.alive_objects);
  CHECK(g_alive_gs.empty(), "capture shaders released (%zu)", g_alive_gs.size());
  CHECK(g_under_lock == 0, "no graphics call under the lock (%d)", g_under_lock);

  std::printf("%s (%d failures)\n", g_failures == 0 ? "PASS" : "FAILED", g_failures);
  return g_failures == 0 ? 0 : 1;
}
