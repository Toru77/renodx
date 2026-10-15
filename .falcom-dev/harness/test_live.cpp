// Native harness: live BVH. Mesh store streaming under the triangle budget,
// arena growth, TLAS rebuild timing and reasons, region moves, retirement,
// garbage reset, GPU allocation failure and retry, switches, failed meshes,
// the lock rule, the GPU contents check, and a CPU port of the trace shader
// run over the uploaded buffers and compared with brute force.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <random>
#include <set>
#include <string>
#include <thread>
#include "gen/mock_base.hpp"
#include "harness_timer.hpp"
#include "json_check.hpp"
#include "src/games/falcomengine-plus/world/bvh/bvh_live.hpp"

using namespace reshade::api;
namespace bvh = falcom_world::bvh;

static int g_failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++g_failures; std::printf("FAIL %s:%d %s | ", __FILE__, __LINE__, #cond); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)
static void Stage(const char* fmt, ...) {
  char msg[512];
  va_list args; va_start(args, fmt); std::vsnprintf(msg, sizeof(msg), fmt, args); va_end(args);
  HStage("frame %u: %s", static_cast<unsigned>(falcom_world::g_state.frame.load()), msg);
}

static int g_under_lock = 0;
static void ProbeLock() {
  if (bvh::g_pool.mutex.try_lock()) {
    bvh::g_pool.mutex.unlock();
  } else {
    ++g_under_lock;
  }
}

struct Res { resource_desc desc; std::vector<uint8_t> bytes; };
struct Device : mock::DeviceBase {
  std::map<uint64_t, Res> res;
  std::map<uint64_t, uint64_t> views;  // view -> resource
  uint64_t next = 0x1000;
  uint64_t next_view = 0x900000;
  int fail_gpu_creates = 0;  // fail the next N gpu_only creations
  int bad_updates = 0;
  int bad_copies = 0;
  int creates = 0;
  device_api get_api() const override { return device_api::d3d11; }
  bool create_resource(const resource_desc &desc, const subresource_data *initial_data, resource_usage, resource *out, void ** = nullptr) override {
    ProbeLock();
    if (desc.heap == memory_heap::gpu_only && fail_gpu_creates > 0) {
      --fail_gpu_creates;
      return false;
    }
    ++creates;
    Res r; r.desc = desc; r.bytes.assign(desc.buffer.size, 0xCD);
    if (initial_data && initial_data->data) std::memcpy(r.bytes.data(), initial_data->data, desc.buffer.size);
    out->handle = next; next += 0x100; res[out->handle] = std::move(r);
    return true;
  }
  void destroy_resource(resource r) override { ProbeLock(); res.erase(r.handle); }
  // Like ReShade's D3D11 convert_resource_desc: buffers report stride 0.
  resource_desc get_resource_desc(resource r) const override {
    auto it = res.find(r.handle);
    if (it == res.end()) return resource_desc{};
    resource_desc desc = it->second.desc;
    if (desc.type == resource_type::buffer) desc.buffer.stride = 0u;
    return desc;
  }
  int structured_mismatch_copies = 0;  // copies between buffers of different element strides
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
  // Same as ReShade's D3D11 device_impl::update_buffer_region: with
  // dst_offset 0 it passes no box to UpdateSubresource, which then writes the
  // whole buffer and reads the buffer's full size from `data`.
  int whole_buffer_updates = 0;
  void update_buffer_region(const void *data, resource dest, uint64_t dest_offset, uint64_t size) override {
    ProbeLock();
    auto it = res.find(dest.handle);
    if (it == res.end()) { ++bad_updates; return; }
    if (dest_offset == 0u) {
      ++whole_buffer_updates;
      size = it->second.bytes.size();
    }
    if (dest_offset + size > it->second.bytes.size()) { ++bad_updates; return; }
    std::memcpy(it->second.bytes.data() + dest_offset, data, size);
  }
  const Res* ViewRes(resource_view v) const {
    auto it = views.find(v.handle); if (it == views.end()) return nullptr;
    auto r = res.find(it->second); return r == res.end() ? nullptr : &r->second;
  }
};
struct CmdList : mock::CommandListBase {
  Device* dev = nullptr;
  device *get_device() override { return dev; }
  void copy_buffer_region(resource src, uint64_t so, resource dst, uint64_t dof, uint64_t size) override {
    ProbeLock();
    auto s = dev->res.find(src.handle), d = dev->res.find(dst.handle);
    if (s == dev->res.end() || d == dev->res.end() || so + size > s->second.bytes.size() || dof + size > d->second.bytes.size()) { dev->bad_copies++; return; }
    if (s->second.desc.heap == memory_heap::gpu_only && d->second.desc.heap == memory_heap::gpu_only
        && s->second.desc.buffer.stride != d->second.desc.buffer.stride) {
      dev->structured_mismatch_copies++;
    }
    std::memcpy(d->second.bytes.data() + dof, s->second.bytes.data() + so, size);
  }
};
struct Queue : mock::QueueBase {
  Device* dev = nullptr; CmdList* cl = nullptr;
  device *get_device() override { return dev; }
  command_list *get_immediate_command_list() override { return cl; }
};

// ---------------------------------------------------------------------------
// Geometry and pool setup through the pool's own functions.

static bvh::PoolDecodedMesh Grid(uint32_t nx, uint32_t nz, float size, float bump) {
  bvh::PoolDecodedMesh m;
  for (uint32_t z = 0; z <= nz; ++z) {
    for (uint32_t x = 0; x <= nx; ++x) {
      const float fx = size * static_cast<float>(x) / static_cast<float>(nx) - size * 0.5f;
      const float fz = size * static_cast<float>(z) / static_cast<float>(nz) - size * 0.5f;
      m.positions.push_back({fx, bump * std::sin(fx * 0.7f) * std::cos(fz * 0.5f), fz});
    }
  }
  for (uint32_t z = 0; z < nz; ++z) {
    for (uint32_t x = 0; x < nx; ++x) {
      const uint32_t a = z * (nx + 1u) + x, b = a + 1u, c = a + nx + 1u, d = c + 1u;
      m.triangles.push_back({a, c, b});
      m.triangles.push_back({b, c, d});
    }
  }
  return m;
}

static bvh::PoolDecodedMesh BoxMesh(float s) {
  bvh::PoolDecodedMesh m;
  for (int i = 0; i < 8; ++i) m.positions.push_back({(i & 1) ? s : -s, (i & 2) ? s : -s, (i & 4) ? s : -s});
  const uint32_t t[12][3] = {{0,1,2},{1,3,2},{4,6,5},{5,6,7},{0,2,4},{2,6,4},{1,5,3},{3,5,7},{0,4,1},{1,4,5},{2,3,6},{3,7,6}};
  for (auto& tri : t) m.triangles.push_back({tri[0], tri[1], tri[2]});
  return m;
}

static void AddMesh(uint64_t key, bvh::PoolDecodedMesh mesh) {
  mesh.bbox_min = mesh.positions[0];
  mesh.bbox_max = mesh.positions[0];
  for (const auto& p : mesh.positions) for (int k = 0; k < 3; ++k) {
    mesh.bbox_min[k] = std::min(mesh.bbox_min[k], p[k]);
    mesh.bbox_max[k] = std::max(mesh.bbox_max[k], p[k]);
  }
  std::lock_guard lock(bvh::g_pool.mutex);
  const char* outcome = bvh::ApplyPoolMesh(key, 0x1000u, mesh);
  CHECK(std::string(outcome) == "added", "mesh %llx %s", (unsigned long long)key, outcome);
}

static void World(float x, float y, float z, float scale, float yaw, float* out) {
  const float c = std::cos(yaw) * scale, s = std::sin(yaw) * scale;
  const float m[12] = {c, 0, s, x, 0, scale, 0, y, -s, 0, c, z};
  std::memcpy(out, m, sizeof(m));
}

static uint32_t g_obs_frame = 100000u;
static void AddInstance(uint64_t key, float x, float y, float z, float scale = 1.f, float yaw = 0.f) {
  float w[12];
  World(x, y, z, scale, yaw, w);
  std::lock_guard lock(bvh::g_pool.mutex);
  bvh::ObservePoolInstance(key, 0x1000u, w, g_obs_frame++);
  bvh::ObservePoolInstance(key, 0x1000u, w, g_obs_frame++);
}

static uint64_t BumpCount(bvh::PoolRevisionSource src) {
  std::lock_guard lock(bvh::g_pool.mutex);
  return bvh::g_pool.tlas_timing.revision_bumps[static_cast<size_t>(src)];
}

static void RetireMesh(uint64_t key) {
  std::lock_guard lock(bvh::g_pool.mutex);
  bvh::InvalidatePoolMeshKey(key);
  bvh::CompactPool();
}

static void SetCamera(float x, float y, float z) {
  std::lock_guard lock(falcom_world::g_state.mutex);
  falcom_world::g_state.camera.valid = true;
  falcom_world::g_state.camera.view_inv[3] = x;
  falcom_world::g_state.camera.view_inv[7] = y;
  falcom_world::g_state.camera.view_inv[11] = z;
}

// ---------------------------------------------------------------------------
// CPU port of world_bvh_trace.hlsli, reading the uploaded GPU buffers.

struct F3 { float x, y, z; };
static F3 operator-(F3 a, F3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static F3 operator+(F3 a, F3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
static F3 operator*(F3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
static float Dot(F3 a, F3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static F3 Cross(F3 a, F3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }

template <typename T>
struct View {
  const uint8_t* bytes = nullptr;
  uint32_t count = 0u;  // GetDimensions
  T operator[](uint32_t i) const { T v; std::memcpy(&v, bytes + static_cast<size_t>(i) * sizeof(T), sizeof(T)); return v; }
};
template <typename T>
static View<T> MakeView(const Device& dev, resource_view v) {
  View<T> view;
  const Res* r = dev.ViewRes(v);
  if (r == nullptr) return view;
  view.bytes = r->bytes.data();
  view.count = static_cast<uint32_t>(r->desc.buffer.size / sizeof(T));
  return view;
}

struct F4 { float x, y, z, w; };
struct TraceInputs {
  View<F4> vertices; View<uint32_t> indices; View<bvh::WorldMeshGPU> meshes; View<bvh::WorldInstanceGPU> instances;
  View<uint32_t> active; View<bvh::BVHNodeGPU> blas_nodes; View<bvh::BVHLeafGPU> blas_leaves;
  View<bvh::BVHNodeGPU> tlas_nodes; View<bvh::BVHLeafGPU> tlas_leaves;
  View<bvh::WorldInstanceGPU> dynamic;  // P2b records (world_bvh_trace.hlsli g_trace_dynamic_instances)
  bool dynamic_skip_hidden = false;     // WORLD_TRACE_DYNAMIC_SKIP_HIDDEN (RTAO): records the camera does not show are skipped
};
struct Counters { uint32_t invalid = 0, overflow = 0; uint32_t p2b_tested = 0, p2b_skipped = 0, p2b_box_hits = 0; };

static bool Aabb(F3 o, F3 d, const float* mn, const float* mx, float t_min, float t_max) {
  const F3 inv = {1.f / d.x, 1.f / d.y, 1.f / d.z};
  const float t0x = (mn[0] - o.x) * inv.x, t1x = (mx[0] - o.x) * inv.x;
  const float t0y = (mn[1] - o.y) * inv.y, t1y = (mx[1] - o.y) * inv.y;
  const float t0z = (mn[2] - o.z) * inv.z, t1z = (mx[2] - o.z) * inv.z;
  const float near_ = std::fmax(std::fmax(std::fmin(t0x, t1x), std::fmin(t0y, t1y)), std::fmax(std::fmin(t0z, t1z), t_min));
  const float far_ = std::fmin(std::fmin(std::fmax(t0x, t1x), std::fmax(t0y, t1y)), std::fmin(std::fmax(t0z, t1z), t_max));
  return near_ <= far_;
}
static bool Tri(F3 o, F3 d, F3 v0, F3 v1, F3 v2, float* t) {
  const F3 e1 = v1 - v0, e2 = v2 - v0, p = Cross(d, e2);
  const float det = Dot(e1, p);
  if (std::fabs(det) < 1e-12f) return false;
  const float inv = 1.f / det;
  const F3 tv = o - v0;
  const float u = Dot(tv, p) * inv;
  if (u < 0.f || u > 1.f) return false;
  const F3 q = Cross(tv, e1);
  const float v = Dot(d, q) * inv;
  if (v < 0.f || u + v > 1.f) return false;
  const float ht = Dot(e2, q) * inv;
  if (ht <= 0.f) return false;
  *t = ht;
  return true;
}
// Transcription of world_bvh_trace.hlsli CameraFadeInterval (checked against
// bvh::CameraFadeInterval below).
static bool HlslCameraFadeInterval(float start, float inv_range, float floor_value, float* lo, float* hi) {
  *lo = 0.0f;
  *hi = 3.0e38f;
  if (floor_value >= 0.5f) return true;
  if (inv_range > 0.0f) { *lo = start + 0.5f / inv_range; return true; }
  if (inv_range < 0.0f) { *hi = start + 0.5f / inv_range; if (*hi >= 0.0f) return true; }
  *lo = 3.0e38f;
  *hi = -1.0f;
  return false;
}
struct Fade { bool hide = false; float floor_value = 0.f; };
static bool TraceBlas(const TraceInputs& in, uint32_t mesh_id, F3 o, F3 d, float t_min, float* t_max, Counters& c,
                      float shown_min = 0.f, float shown_max = 3.0e38f, bool hide = false, float* faded_t = nullptr,
                      uint32_t* out_faded = nullptr) {
  if (out_faded) *out_faded = 0u;
  const bvh::WorldMeshGPU mesh = in.meshes[mesh_id];
  const uint32_t leaf_count = (uint32_t)mesh.build[3];
  if (leaf_count == 0u) return false;
  const uint32_t node_count = 2u * leaf_count - 1u, node_base = (uint32_t)mesh.build[0], leaf_base = (uint32_t)mesh.build[2];
  const uint32_t vo = (uint32_t)mesh.header[0], vc = (uint32_t)mesh.header[1], io = (uint32_t)mesh.header[2], ic = (uint32_t)mesh.header[3];
  uint32_t stack[64]; uint32_t sp = 0, node = 0; bool found = false;
  while (true) {
    if (node >= node_count) { c.invalid++; }
    else {
      const bvh::BVHNodeGPU n = in.blas_nodes[node_base + node];
      if (Aabb(o, d, n.bounds_min, n.bounds_max, t_min, *t_max)) {
        if (n.child_or_leaf & bvh::kBvhLeafFlag) {
          const uint32_t li = n.child_or_leaf & 0x7FFFFFFFu;
          if (li >= leaf_count) c.invalid++;
          else {
            const uint32_t prim = in.blas_leaves[leaf_base + li].prim;
            if (prim * 3u + 2u >= ic) c.invalid++;
            else {
              const uint32_t i0 = in.indices[io + prim * 3u], i1 = in.indices[io + prim * 3u + 1u], i2 = in.indices[io + prim * 3u + 2u];
              if (i0 >= vc || i1 >= vc || i2 >= vc) c.invalid++;
              else {
                const F4 a = in.vertices[vo + i0], b = in.vertices[vo + i1], e = in.vertices[vo + i2];
                float tt;
                if (Tri(o, d, {a.x, a.y, a.z}, {b.x, b.y, b.z}, {e.x, e.y, e.z}, &tt) && tt >= t_min && tt < *t_max) {
                  const bool shown = tt >= shown_min && tt <= shown_max;
                  if (shown || !hide) { *t_max = tt; found = true; if (out_faded) *out_faded = shown ? 0u : 1u; }
                  else if (faded_t) *faded_t = std::fmin(*faded_t, tt);
                }
              }
            }
          }
        } else {
          const uint32_t l = n.child_or_leaf, r = n.sibling_or_right;
          if (l >= node_count || r >= node_count || l == r) c.invalid++;
          else if (sp < 64) { stack[sp++] = r; node = l; continue; }
          else c.overflow++;
        }
      }
    }
    if (sp == 0) break;
    node = stack[--sp];
  }
  return found;
}
// One instance (a TLAS leaf or a P2b record) in its local space: TraceBlas with the instance's camera interval
// (world_bvh_trace.hlsli TraceWorldInstance). Updates best when it is hit nearer.
static bool TraceInstance(const TraceInputs& in, const bvh::WorldInstanceGPU& inst, uint32_t mesh_id, F3 o, F3 d, Fade fade,
                          Counters& c, float* best, float* faded_t, uint32_t* best_faded) {
  const float* iw = inst.inverse_world;
  const F3 lo = {o.x * iw[0] + o.y * iw[1] + o.z * iw[2] + iw[3], o.x * iw[4] + o.y * iw[5] + o.z * iw[6] + iw[7], o.x * iw[8] + o.y * iw[9] + o.z * iw[10] + iw[11]};
  const F3 ld = {d.x * iw[0] + d.y * iw[1] + d.z * iw[2], d.x * iw[4] + d.y * iw[5] + d.z * iw[6], d.x * iw[8] + d.y * iw[9] + d.z * iw[10]};
  float shown_min = 0.f, shown_max = 3.0e38f;
  const uint32_t visibility_flags = static_cast<uint32_t>(inst.visibility[2] + 0.5f);
  if ((visibility_flags & 2u) == 0u) { shown_min = 3.0e38f; shown_max = -1.0f; }
  else if ((visibility_flags & 1u) != 0u) HlslCameraFadeInterval(inst.visibility[0], inst.visibility[1], fade.floor_value, &shown_min, &shown_max);
  float local_best = *best;
  uint32_t local_faded = 0u;
  if (!TraceBlas(in, mesh_id, lo, ld, 0.001f, &local_best, c, shown_min, shown_max, fade.hide, faded_t, &local_faded)) return false;
  *best = local_best;
  *best_faded = local_faded;
  return true;
}
static float TraceWorld(const TraceInputs& in, F3 o, F3 d, Counters& c, uint32_t* hit_instance, Fade fade = {},
                        uint32_t* camera_faded = nullptr) {
  *hit_instance = 0xFFFFFFFFu;
  uint32_t best_faded = 0u;
  float faded_t = 3.0e38f;
  if (camera_faded) *camera_faded = 0u;
  const uint32_t tl = in.tlas_leaves.count;
  if (tl == 0) return -1.f;  // the TLAS is never empty in the stages that trace P2b records
  const uint32_t tn = 2u * tl - 1u;
  float best = 1e30f; bool any = false;
  uint32_t stack[64]; uint32_t sp = 0, node = 0;
  while (true) {
    if (node >= tn) c.invalid++;
    else {
      const bvh::BVHNodeGPU n = in.tlas_nodes[node];
      if (Aabb(o, d, n.bounds_min, n.bounds_max, 0.001f, best)) {
        if (n.child_or_leaf & bvh::kBvhLeafFlag) {
          const uint32_t slot = n.child_or_leaf & 0x7FFFFFFFu;
          if (slot >= tl) c.invalid++;
          else {
            const uint32_t is = in.tlas_leaves[slot].prim;
            if (is >= in.active.count) c.invalid++;
            else {
              const uint32_t resolved = in.active[is];
              if (resolved >= in.instances.count) c.invalid++;
              else {
                const bvh::WorldInstanceGPU inst = in.instances[resolved];
                const uint32_t mesh_id = (uint32_t)inst.header[0];
                if (mesh_id >= in.meshes.count) c.invalid++;
                else if (TraceInstance(in, inst, mesh_id, o, d, fade, c, &best, &faded_t, &best_faded)) {
                  any = true; *hit_instance = resolved;
                }
              }
            }
          }
        } else {
          const uint32_t l = n.child_or_leaf, r = n.sibling_or_right;
          if (l >= tn || r >= tn || l == r) c.invalid++;
          else if (sp < 64) { stack[sp++] = r; node = l; continue; }
          else c.overflow++;
        }
      }
    }
    if (sp == 0) break;
    node = stack[--sp];
  }
  // P2b records after the TLAS (world_bvh_trace.hlsli): the same instance trace, the record's world box as the
  // prefilter within best, and the hidden skip when the trace hides what the camera does not show.
  for (uint32_t r = 0; r < in.dynamic.count; ++r) {
    const bvh::WorldInstanceGPU inst = in.dynamic[r];
    if (inst.header[3] < 0.5f) { c.p2b_skipped++; continue; }
    const uint32_t mesh_id = (uint32_t)inst.header[0];
    if (mesh_id >= in.meshes.count) { c.invalid++; c.p2b_skipped++; continue; }
    if (fade.hide && in.dynamic_skip_hidden && (static_cast<uint32_t>(inst.visibility[2] + 0.5f) & 2u) == 0u) continue;
    c.p2b_tested++;
    if (!Aabb(o, d, inst.bounds_min, inst.bounds_max, 0.001f, best)) continue;
    c.p2b_box_hits++;
    if (TraceInstance(in, inst, mesh_id, o, d, fade, c, &best, &faded_t, &best_faded)) {
      any = true; *hit_instance = 0x40000000u | r;
    }
  }
  if (camera_faded) *camera_faded = (best_faded != 0u || faded_t < best) ? 1u : 0u;
  return any ? best : -1.f;
}

static TraceInputs Inputs(const Device& dev, const bvh::BvhDeviceData& data) {
  TraceInputs in;
  in.vertices = MakeView<F4>(dev, data.vertices.srv);
  in.indices = MakeView<uint32_t>(dev, data.indices.srv);
  in.meshes = MakeView<bvh::WorldMeshGPU>(dev, data.mesh_srv);
  in.instances = MakeView<bvh::WorldInstanceGPU>(dev, data.instance_srv);
  in.active = MakeView<uint32_t>(dev, data.active_srv);
  in.blas_nodes = MakeView<bvh::BVHNodeGPU>(dev, data.blas_nodes.srv);
  in.blas_leaves = MakeView<bvh::BVHLeafGPU>(dev, data.blas_leaves.srv);
  in.tlas_nodes = MakeView<bvh::BVHNodeGPU>(dev, data.tlas_node_srv);
  in.tlas_leaves = MakeView<bvh::BVHLeafGPU>(dev, data.tlas_leaf_srv);
  return in;
}

// The P2b records of every flagged, non-alpha mesh in the region, as world_dynamic_expand.cs_5_0.hlsl writes them: live
// slot, world rows, closed-form inverse, world box of the local box, draw flags (the shader writes no near fade).
static std::vector<bvh::WorldInstanceGPU> BuildDynamicRecords(const bvh::BvhDeviceData& data, const bvh::PoolRegion& region) {
  std::vector<bvh::WorldInstanceGPU> records;
  std::lock_guard lock(bvh::g_pool.mutex);
  for (const auto& inst : bvh::g_pool.instances) {
    if (!bvh::PoolInstanceInRegion(inst, region)) continue;
    const auto& mesh = bvh::g_pool.meshes[inst.mesh_id];
    const auto slot = data.slot_by_uid.find(mesh.uid);
    if (!mesh.dynamic || mesh.alpha || slot == data.slot_by_uid.end()) continue;
    const auto& local = data.slots[slot->second];
    bvh::WorldInstanceGPU r = {};
    r.header[0] = static_cast<float>(slot->second);
    r.header[3] = 1.f;
    std::memcpy(r.world, inst.matrix, sizeof(float) * 12u);
    r.world[15] = 1.f;
    falcom_world::ComputeMatrixInverse(inst.matrix, r.inverse_world);
    float lo[3] = {1e30f, 1e30f, 1e30f}, hi[3] = {-1e30f, -1e30f, -1e30f};
    for (int corner = 0; corner < 8; ++corner) {
      const float p[3] = {(corner & 1) ? local.bbox_max[0] : local.bbox_min[0], (corner & 2) ? local.bbox_max[1] : local.bbox_min[1],
                          (corner & 4) ? local.bbox_max[2] : local.bbox_min[2]};
      float w[3];
      falcom_world::TransformInst4x3RowDot(inst.matrix, p[0], p[1], p[2], w);
      for (int k = 0; k < 3; ++k) {
        lo[k] = std::min(lo[k], w[k]);
        hi[k] = std::max(hi[k], w[k]);
      }
    }
    for (int k = 0; k < 3; ++k) {
      r.bounds_min[k] = lo[k];
      r.bounds_max[k] = hi[k];
    }
    const bvh::PoolCameraVisibility vis = bvh::GetPoolCameraVisibility(inst);
    r.visibility[2] = static_cast<float>((vis.camera_seen ? bvh::kInstanceCameraVisible : 0u) | (vis.light_seen ? bvh::kInstanceShadowCaster : 0u));
    records.push_back(r);
  }
  return records;
}

// Brute force over the pool's instances in the region whose mesh is on the GPU.
struct RefInstance { std::vector<F3> verts; bvh::PoolCameraVisibility vis; };  // 3 verts per triangle
static std::vector<RefInstance> BuildRefInstances(const bvh::BvhDeviceData& data, const bvh::PoolRegion& region) {
  std::lock_guard lock(bvh::g_pool.mutex);
  std::vector<RefInstance> refs;
  for (const auto& inst : bvh::g_pool.instances) {
    if (!bvh::PoolInstanceInRegion(inst, region)) continue;
    const auto& mesh = bvh::g_pool.meshes[inst.mesh_id];
    const auto slot = data.slot_by_uid.find(mesh.uid);
    if (slot == data.slot_by_uid.end() || slot->second >= data.descriptor_count) continue;
    RefInstance& ref = refs.emplace_back();
    ref.vis = bvh::GetPoolCameraVisibility(inst);
    for (size_t t = 0; t + 2 < mesh.indices.size(); t += 3) {
      if (mesh.indices[t] >= mesh.positions.size() || mesh.indices[t + 1] >= mesh.positions.size()
          || mesh.indices[t + 2] >= mesh.positions.size()) {
        continue;  // like BuildMeshBlas: such triangles are not in the BVH
      }
      for (int k = 0; k < 3; ++k) {
        const auto& p = mesh.positions[mesh.indices[t + k]];
        float w[3]; falcom_world::TransformInst4x3RowDot(inst.matrix, p[0], p[1], p[2], w);
        ref.verts.push_back({w[0], w[1], w[2]});
      }
    }
  }
  return refs;
}

static float BruteForce(F3 o, F3 d, const std::vector<RefInstance>& refs) {
  float best = 1e30f; bool any = false;
  for (const auto& ref : refs) {
    for (size_t i = 0; i + 2 < ref.verts.size(); i += 3) {
      float tt;
      if (Tri(o, d, ref.verts[i], ref.verts[i + 1], ref.verts[i + 2], &tt) && tt >= 0.001f && tt < best) { best = tt; any = true; }
    }
  }
  return any ? best : -1.f;
}

// Brute-force reference cache: one (t, faded) result per ray in temp/pooltest_cache/<key>.bin.
// The key covers the version, platform, ray directions, origin, fade and every reference triangle.
// A hit is spot-checked on 8 evenly spaced rays; a mismatch deletes the file and fails the run.
static const uint32_t kRefCacheVersion = 1;  // bump when the reference or the ray generation changes
static const char kPlatformTag[] = "windows-x64";
static int g_cache_hits = 0, g_cache_misses = 0;
static const bool g_quick = std::getenv("FALCOM_QUICK") != nullptr;  // rays / 5: not a full verification
struct RefResult { float t; uint32_t faded; };
static float BruteForceFade(F3 o, F3 d, const std::vector<RefInstance>& refs, Fade fade, uint32_t* faded);

static void HashBytes(uint64_t* h, const void* data, size_t n) {
  const auto* p = static_cast<const unsigned char*>(data);
  for (size_t i = 0; i < n; ++i) { *h ^= p[i]; *h *= 1099511628211ull; }
}
static uint64_t RefCacheKey(const std::vector<F3>& dirs, F3 origin, uint32_t seed, const std::vector<RefInstance>& refs,
                            const Fade* fade) {
  uint64_t h = 14695981039346656037ull;
  HashBytes(&h, &kRefCacheVersion, sizeof kRefCacheVersion);
  HashBytes(&h, kPlatformTag, sizeof kPlatformTag);
  const uint64_t rays = dirs.size();
  HashBytes(&h, &rays, sizeof rays);
  HashBytes(&h, dirs.data(), dirs.size() * sizeof(F3));
  HashBytes(&h, &seed, sizeof seed);
  HashBytes(&h, &origin, sizeof origin);
  const uint32_t fade_mode = fade ? (fade->hide ? 2u : 1u) : 0u;
  const float floor_value = fade ? fade->floor_value : 0.f;
  HashBytes(&h, &fade_mode, sizeof fade_mode);
  HashBytes(&h, &floor_value, sizeof floor_value);
  for (const auto& ref : refs) {
    const uint64_t verts = ref.verts.size();
    HashBytes(&h, &verts, sizeof verts);
    HashBytes(&h, ref.verts.data(), verts * sizeof(F3));
    const uint32_t vis_bits = (ref.vis.camera_seen ? 1u : 0u) | (ref.vis.near_fade ? 2u : 0u);
    HashBytes(&h, &vis_bits, sizeof vis_bits);
    HashBytes(&h, ref.vis.inputs.param, sizeof ref.vis.inputs.param);
  }
  return h;
}
static std::filesystem::path RefCachePath(uint64_t key) {
  char name[32];
  std::snprintf(name, sizeof name, "%016llx.bin", static_cast<unsigned long long>(key));
  return std::filesystem::temp_directory_path() / "pooltest_cache" / name;
}
static bool ReadRefCache(const std::filesystem::path& path, uint64_t key, size_t n, std::vector<RefResult>* out) {
  std::ifstream file(path, std::ios::binary);
  uint64_t stored_key = 0, stored_n = 0;
  file.read(reinterpret_cast<char*>(&stored_key), sizeof stored_key);
  file.read(reinterpret_cast<char*>(&stored_n), sizeof stored_n);
  if (!file || stored_key != key || stored_n != n) return false;
  out->resize(n);
  file.read(reinterpret_cast<char*>(out->data()), n * sizeof(RefResult));
  return static_cast<bool>(file);
}
static void WriteRefCache(const std::filesystem::path& path, uint64_t key, const std::vector<RefResult>& results) {
  std::filesystem::create_directories(path.parent_path());
  const std::filesystem::path tmp = path.string() + ".tmp";
  {
    std::ofstream file(tmp, std::ios::binary);
    const uint64_t n = results.size();
    file.write(reinterpret_cast<const char*>(&key), sizeof key);
    file.write(reinterpret_cast<const char*>(&n), sizeof n);
    file.write(reinterpret_cast<const char*>(results.data()), n * sizeof(RefResult));
  }
  std::filesystem::rename(tmp, path);
}
// Each worker fills its own result slots, so the output does not depend on the worker count.
template <typename Eval>
static std::vector<RefResult> EvalRefs(size_t n, Eval eval) {
  std::vector<RefResult> out(n);
  const size_t workers = std::clamp<size_t>(std::thread::hardware_concurrency(), 1, 8);
  std::vector<std::thread> threads;
  for (size_t w = 0; w < workers; ++w) {
    threads.emplace_back([&, w] {
      for (size_t i = w; i < n; i += workers) out[i] = eval(i);
    });
  }
  for (auto& t : threads) t.join();
  return out;
}
// Reference results for every direction: from the cache when the key matches, else computed and stored.
static std::vector<RefResult> RefResults(const std::vector<F3>& dirs, F3 origin, uint32_t seed,
                                         const std::vector<RefInstance>& refs, const Fade* fade) {
  auto eval = [&](size_t i) {
    RefResult r{0.f, 0u};
    if (fade) {
      r.t = BruteForceFade(origin, dirs[i], refs, *fade, &r.faded);
    } else {
      r.t = BruteForce(origin, dirs[i], refs);
    }
    return r;
  };
  const size_t n = dirs.size();
  const char* mode = std::getenv("FALCOM_REF_CACHE");
  if (mode && std::string(mode) == "off") return EvalRefs(n, eval);
  const uint64_t key = RefCacheKey(dirs, origin, seed, refs, fade);
  const std::filesystem::path path = RefCachePath(key);
  std::vector<RefResult> cached;
  if (ReadRefCache(path, key, n, &cached)) {
    bool agree = true;
    for (size_t k = 0; k < 8 && n > 0; ++k) {
      const size_t i = k * n / 8;
      const RefResult r = eval(i);
      agree = agree && r.t == cached[i].t && r.faded == cached[i].faded;
    }
    if (agree) {
      ++g_cache_hits;
      return cached;
    }
    std::filesystem::remove(path);
    std::printf("  removed %s\n", path.string().c_str());
    std::vector<RefResult> fresh = EvalRefs(n, eval);
    WriteRefCache(path, key, fresh);
    CHECK(false, "stale ref cache");
    return fresh;
  }
  ++g_cache_misses;
  std::vector<RefResult> results = EvalRefs(n, eval);
  WriteRefCache(path, key, results);
  return results;
}

// Rays at triangle centroids of random region instances, plus random rays.
static int CompareTraces(const Device& dev, const bvh::BvhDeviceData& data, const char* label, int rays, uint32_t seed,
                         const std::vector<bvh::WorldInstanceGPU>* records = nullptr, Fade fade = {}, bool skip_hidden = false) {
  if (g_quick) rays /= 5;
  const bvh::PoolRegion region = bvh::CurrentPoolRegion();
  TraceInputs in = Inputs(dev, data);
  if (records != nullptr) {
    in.dynamic.bytes = reinterpret_cast<const uint8_t*>(records->data());
    in.dynamic.count = static_cast<uint32_t>(records->size());
  }
  in.dynamic_skip_hidden = skip_hidden;
  std::mt19937 rng(seed);
  std::uniform_real_distribution<float> uni(-1.f, 1.f);
  float cam[3]; bvh::PoolCameraPosition(cam);
  const F3 origin = {cam[0], cam[1] + 30.f, cam[2]};
  int mismatches = 0, hits = 0, p2b_wins = 0;
  Counters c;
  std::vector<F3> targets;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    for (const auto& inst : bvh::g_pool.instances) {
      if (!bvh::PoolInstanceInRegion(inst, region)) continue;
      const auto& mesh = bvh::g_pool.meshes[inst.mesh_id];
      for (int k = 0; k < 4; ++k) {
        const size_t t = (rng() % (mesh.indices.size() / 3)) * 3;
        // The broken test mesh H has indices past its vertices: no target there.
        if (mesh.indices[t] >= mesh.positions.size() || mesh.indices[t + 1] >= mesh.positions.size()
            || mesh.indices[t + 2] >= mesh.positions.size()) {
          continue;
        }
        F3 centroid = {0, 0, 0};
        for (int j = 0; j < 3; ++j) {
          const auto& p = mesh.positions[mesh.indices[t + j]];
          float w[3]; falcom_world::TransformInst4x3RowDot(inst.matrix, p[0], p[1], p[2], w);
          centroid = centroid + F3{w[0], w[1], w[2]} * (1.f / 3.f);
        }
        targets.push_back(centroid);
      }
    }
  }
  std::vector<F3> dirs;
  for (int i = 0; i < rays; ++i) {
    F3 dir;
    if (!targets.empty() && i % 2 == 0) {
      dir = targets[rng() % targets.size()] - origin;
    } else {
      dir = {uni(rng), uni(rng) - 0.3f, uni(rng)};
    }
    const float len = std::sqrt(Dot(dir, dir));
    if (len < 1e-6f) continue;
    dirs.push_back(dir * (1.f / len));
  }
  const std::vector<RefInstance> refs = BuildRefInstances(data, region);
  Stage("%s: start, %d rays", label, rays);
  const std::vector<RefResult> ref = RefResults(dirs, origin, seed, refs, fade.hide ? &fade : nullptr);
  for (size_t i = 0; i < dirs.size(); ++i) {
    if (i % 50 == 0) HProgress("  %s: ray %zu/%zu", label, i, dirs.size());
    uint32_t instance;
    const float t_bvh = TraceWorld(in, origin, dirs[i], c, &instance, fade);
    const float t_ref = ref[i].t;
    if (t_bvh >= 0.f && (instance & 0xC0000000u) == 0x40000000u) ++p2b_wins;
    const bool agree = (t_bvh < 0.f && t_ref < 0.f)
                       || (t_bvh >= 0.f && t_ref >= 0.f && std::fabs(t_bvh - t_ref) <= 1e-3f * std::max(1.f, t_ref));
    if (t_ref >= 0.f) ++hits;
    if (!agree) {
      if (mismatches < 3) std::printf("  %s ray %zu: bvh %.5f ref %.5f\n", label, i, t_bvh, t_ref);
      ++mismatches;
    }
  }
  std::printf("%s: %d rays, %d hits, %d mismatches, invalid %u overflow %u\n", label, rays, hits, mismatches, c.invalid, c.overflow);
  CHECK(c.invalid == 0 && c.overflow == 0, "%s trace counters", label);
  CHECK(hits > rays / 4, "%s enough hits (%d)", label, hits);
  if (records != nullptr) {
    // Every ray tests or skips every record (no hidden skip here), boxes are a subset of the tests, wins are rays.
    CHECK(c.p2b_tested + c.p2b_skipped == dirs.size() * records->size(), "%s P2b tested %u + skipped %u != rays %zu * records %zu",
          label, c.p2b_tested, c.p2b_skipped, dirs.size(), records->size());
    CHECK(c.p2b_box_hits <= c.p2b_tested, "%s P2b box hits %u > tested %u", label, c.p2b_box_hits, c.p2b_tested);
    CHECK(static_cast<size_t>(p2b_wins) <= dirs.size(), "%s P2b wins %d > rays %zu", label, p2b_wins, dirs.size());
  }
  return mismatches;
}

// Brute force with the game camera fade, from the pool's own rule
// (GetPoolCameraVisibility, CameraFadeApplies, camera_fade.hpp).
static float BruteForceFade(F3 o, F3 d, const std::vector<RefInstance>& refs, Fade fade, uint32_t* faded) {
  float best = 1e30f; bool any = false; bool best_faded = false; float faded_t = 3.0e38f;
  for (const auto& ref : refs) {
    const auto& vis = ref.vis;
    float lo = 0.f, hi = bvh::kCameraFadeNever;
    if (!vis.camera_seen) {
      lo = bvh::kCameraFadeNever;
      hi = -1.f;
    } else if (bvh::CameraFadeApplies(vis)) {
      bvh::CameraFadeInterval(vis.inputs.param[0], vis.inputs.param[1], fade.floor_value, &lo, &hi);
    }
    for (size_t i = 0; i + 2 < ref.verts.size(); i += 3) {
      float tt;
      if (!Tri(o, d, ref.verts[i], ref.verts[i + 1], ref.verts[i + 2], &tt) || tt < 0.001f) continue;
      const bool shown = tt >= lo && tt <= hi;
      if (shown || !fade.hide) {
        if (tt < best) { best = tt; any = true; best_faded = !shown; }
      } else {
        faded_t = std::fmin(faded_t, tt);
      }
    }
  }
  *faded = (best_faded || faded_t < best) ? 1u : 0u;
  return any ? best : -1.f;
}

// Rays from the camera at instance centroids (and random), traced by the CPU
// port of the shader and by brute force, with the camera fade.
static int CompareFadeTraces(const Device& dev, const bvh::BvhDeviceData& data, const char* label, Fade fade, int rays,
                             uint32_t seed, uint32_t* faded_rays) {
  if (g_quick) rays /= 5;
  const bvh::PoolRegion region = bvh::CurrentPoolRegion();
  const TraceInputs in = Inputs(dev, data);
  std::mt19937 rng(seed);
  std::uniform_real_distribution<float> uni(-1.f, 1.f);
  float cam[3]; bvh::PoolCameraPosition(cam);
  const F3 origin = {cam[0], cam[1], cam[2]};
  std::vector<F3> targets;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    for (const auto& inst : bvh::g_pool.instances) {
      if (!bvh::PoolInstanceInRegion(inst, region)) continue;
      targets.push_back({inst.matrix[3], inst.matrix[7], inst.matrix[11]});
    }
  }
  int mismatches = 0;
  *faded_rays = 0u;
  Counters c;
  std::vector<F3> dirs;
  for (int i = 0; i < rays; ++i) {
    F3 dir = (i % 3 != 0 && !targets.empty()) ? targets[rng() % targets.size()] - origin + F3{uni(rng), uni(rng), uni(rng)}
                                              : F3{uni(rng), uni(rng) - 0.3f, uni(rng)};
    const float len = std::sqrt(Dot(dir, dir));
    if (len < 1e-6f) continue;
    dirs.push_back(dir * (1.f / len));
  }
  const std::vector<RefInstance> refs = BuildRefInstances(data, region);
  Stage("%s: start, %d rays", label, rays);
  const std::vector<RefResult> ref = RefResults(dirs, origin, seed, refs, &fade);
  for (size_t i = 0; i < dirs.size(); ++i) {
    if (i % 50 == 0) HProgress("  %s: ray %zu/%zu", label, i, dirs.size());
    uint32_t instance, faded_bvh = 0u;
    const float t_bvh = TraceWorld(in, origin, dirs[i], c, &instance, fade, &faded_bvh);
    const float t_ref = ref[i].t;
    const uint32_t faded_ref = ref[i].faded;
    const bool agree = ((t_bvh < 0.f && t_ref < 0.f)
                        || (t_bvh >= 0.f && t_ref >= 0.f && std::fabs(t_bvh - t_ref) <= 1e-3f * std::max(1.f, t_ref)))
                       && faded_bvh == faded_ref;
    *faded_rays += faded_ref;
    if (!agree) {
      if (mismatches < 3) std::printf("  %s ray %zu: bvh %.5f/%u ref %.5f/%u\n", label, i, t_bvh, faded_bvh, t_ref, faded_ref);
      ++mismatches;
    }
  }
  std::printf("%s: %d rays, %u camera-faded, %d mismatches\n", label, rays, *faded_rays, mismatches);
  CHECK(c.invalid == 0 && c.overflow == 0, "%s trace counters", label);
  return mismatches;
}

static void AddFadedInstance(uint64_t key, float x, float y, float z, float start, float inv_range, bool near_fade_ps) {
  float w[12];
  World(x, y, z, 1.f, 0.3f, w);
  bvh::PoolVisibility vis;
  vis.valid = true;
  vis.color[3] = 1.f;
  vis.param[0] = start;
  vis.param[1] = inv_range;
  const bvh::PoolSighting sighting{
      static_cast<uint8_t>(bvh::kPoolPassCamera | bvh::kPoolPassVisibility | (near_fade_ps ? bvh::kPoolPassNearFadePs : 0u)), 0x5150u};
  std::lock_guard lock(bvh::g_pool.mutex);
  bvh::ObservePoolInstance(key, 0x1000u, w, g_obs_frame++, &sighting, &vis);
  bvh::ObservePoolInstance(key, 0x1000u, w, g_obs_frame++, &sighting, &vis);
}

// Moves the dynamic instance admitted through `key` to (x, 0, z), the way
// FollowPoolMovingInstance does: pose, inverse, bounds, admitted key, revision.
static void MoveDynamicInstance(uint64_t key, float x, float z, uint32_t frame) {
  float w[12];
  World(x, 0.f, z, 1.f, 0.f, w);
  std::lock_guard lock(bvh::g_pool.mutex);
  for (bvh::WorldInstance& inst : bvh::g_pool.instances) {
    if (inst.mesh_key != key) continue;
    float mn[3], mx[3];
    bvh::ComputePoolInstanceBounds(bvh::g_pool.meshes[inst.mesh_id], w, mn, mx);
    bvh::g_pool.admitted_keys.erase(bvh::PoolAdmittedKey(inst.mesh_id, falcom_world::MatrixHash(inst.matrix, 12)));
    std::memcpy(inst.matrix, w, sizeof(w));
    falcom_world::ComputeMatrixInverse(inst.matrix, inst.inverse_world);
    std::memcpy(inst.bounds_min, mn, sizeof(mn));
    std::memcpy(inst.bounds_max, mx, sizeof(mx));
    bvh::g_pool.admitted_keys.insert(bvh::PoolAdmittedKey(inst.mesh_id, falcom_world::MatrixHash(inst.matrix, 12)));
    if (!inst.dynamic) bvh::g_pool.revision += 1u;  // as FollowPoolMovingInstance: a new dynamic flag re-partitions at a rebuild
    inst.dynamic = true;
    inst.last_follow_frame = frame;
    bvh::g_pool.dynamic_revision += 1u;
    return;
  }
}

// Moves the instance admitted through `key` from the pose (from_x, 0, from_z) to
// (to_x, 0, to_z) through FollowPoolMovingInstance itself, with a Moving verdict.
static void FollowInstance(uint64_t key, float from_x, float from_z, float to_x, float to_z, uint32_t frame) {
  float prev[12], world[12];
  World(from_x, 0.f, from_z, 1.f, 0.f, prev);
  World(to_x, 0.f, to_z, 1.f, 0.f, world);
  std::lock_guard lock(bvh::g_pool.mutex);
  bvh::FollowPoolMovingInstance(key, 0x1000u, world, prev, true, frame, bvh::PoolSightingVerdict::Moving);
}

struct F2 {
  float x;
  float y;
};

// Step 4 (alpha-tested foliage, CPU transcriptions): IntersectTriangle with barycentrics, AlphaCutHit,
// the nearest non-cut hit (the any-hit semantics of TraceBlas), the blit's packing, and the TLAS descriptor's
// material slot. The GPU side is compiled (FXC) and runs in game; these checks transcribe its arithmetic.
static bool TriBary(F3 o, F3 d, F3 v0, F3 v1, F3 v2, float* t, float* bu, float* bv) {
  const F3 edge1 = v1 - v0;
  const F3 edge2 = v2 - v0;
  const F3 p = Cross(d, edge2);
  const float det = Dot(edge1, p);
  if (std::fabs(det) < 1e-12f) return false;
  const float inv_det = 1.f / det;
  const F3 tv = o - v0;
  const float u = Dot(tv, p) * inv_det;
  if (u < 0.f || u > 1.f) return false;
  const F3 q = Cross(tv, edge1);
  const float v = Dot(d, q) * inv_det;
  if (v < 0.f || u + v > 1.f) return false;
  const float hit_t = Dot(edge2, q) * inv_det;
  if (hit_t <= 0.f) return false;
  *t = hit_t;
  *bu = u;
  *bv = v;
  return true;
}

// The atlas slice as the GPU reads it: 256 words per row, 256 rows; texel x is byte x & 3 of word x >> 2.
// Transcribes AlphaCutHit (world_bvh_trace.hlsli) after the UV lookup: true when the texel is cut.
static bool AlphaCutCpu(const std::vector<uint32_t>& slice, float threshold, F2 scroll, F2 uv0, F2 uv1, F2 uv2, float bu, float bv) {
  const float uvx = (1.f - bu - bv) * uv0.x + bu * uv1.x + bv * uv2.x;
  const float uvy = (1.f - bu - bv) * uv0.y + bu * uv1.y + bv * uv2.y;
  const float tx = uvx + scroll.x;
  const float fx = tx - std::floor(tx);
  const float ty = 1.f - (uvy + scroll.y);
  const float fy = ty - std::floor(ty);
  const uint32_t x = (std::min)(static_cast<uint32_t>(std::floor(fx * 1024.f)), 1023u);
  const uint32_t y = (std::min)(static_cast<uint32_t>(std::floor(fy * 256.f)), 255u);
  const uint32_t word = slice[y * 256u + (x >> 2u)];
  const uint32_t texel = (word >> ((x & 3u) * 8u)) & 0xFFu;
  return static_cast<float>(texel) * (1.f / 255.f) - threshold < 0.f;
}

static void TestAlphaCut() {
  std::mt19937 rng(7);
  std::uniform_real_distribution<float> unit(0.f, 1.f);
  long long hits = 0, cut = 0, bad_bary = 0, bad_nearest = 0;
  for (int scene = 0; scene < 40; ++scene) {
    std::vector<uint32_t> slice(256u * 256u);
    for (uint32_t& word : slice) word = static_cast<uint32_t>(rng());
    const float threshold = unit(rng);
    const F2 scroll = {unit(rng) * 4.f - 2.f, unit(rng) * 4.f - 2.f};
    std::vector<std::array<F3, 3>> tris;
    std::vector<std::array<F2, 3>> uvs;
    for (int i = 0; i < 24; ++i) {
      std::array<F3, 3> tri;
      std::array<F2, 3> uv;
      for (int k = 0; k < 3; ++k) {
        tri[k] = {unit(rng) * 10.f - 5.f, unit(rng) * 10.f - 5.f, unit(rng) * 10.f - 5.f};
        uv[k] = {unit(rng) * 4.f - 2.f, unit(rng) * 4.f - 2.f};
      }
      tris.push_back(tri);
      uvs.push_back(uv);
    }
    for (int r = 0; r < 300; ++r) {
      const F3 o = {unit(rng) * 14.f - 7.f, unit(rng) * 14.f - 7.f, unit(rng) * 14.f - 7.f};
      const F3 target = {unit(rng) * 10.f - 5.f, unit(rng) * 10.f - 5.f, unit(rng) * 10.f - 5.f};
      const F3 d = target - o;
      // Any-hit as TraceBlas runs it: a cut triangle is skipped and leaves t_max alone.
      float best = std::numeric_limits<float>::infinity();
      int best_tri = -1;
      std::vector<std::pair<float, int>> all;
      for (int i = 0; i < static_cast<int>(tris.size()); ++i) {
        float t = 0.f, bu = 0.f, bv = 0.f;
        if (!TriBary(o, d, tris[i][0], tris[i][1], tris[i][2], &t, &bu, &bv)) continue;
        hits += 1;
        all.push_back({t, i});
        const bool is_cut = AlphaCutCpu(slice, threshold, scroll, uvs[i][0], uvs[i][1], uvs[i][2], bu, bv);
        if (is_cut) {
          cut += 1;
          continue;
        }
        if (t < best) {
          best = t;
          best_tri = i;
        }
        // The barycentrics rebuild the hit point.
        const F3 p = o + d * t;
        const F3 q = tris[i][0] * (1.f - bu - bv) + tris[i][1] * bu + tris[i][2] * bv;
        if (std::fabs(p.x - q.x) + std::fabs(p.y - q.y) + std::fabs(p.z - q.z) > 1e-3f) bad_bary += 1;
      }
      // Reference: the nearest hit in t order that is not cut.
      std::sort(all.begin(), all.end());
      int ref_tri = -1;
      for (const auto& hit : all) {
        const int i = hit.second;
        float t = 0.f, bu = 0.f, bv = 0.f;
        TriBary(o, d, tris[i][0], tris[i][1], tris[i][2], &t, &bu, &bv);
        if (!AlphaCutCpu(slice, threshold, scroll, uvs[i][0], uvs[i][1], uvs[i][2], bu, bv)) {
          ref_tri = i;
          break;
        }
      }
      if (ref_tri != best_tri) bad_nearest += 1;
    }
  }
  CHECK(hits > 1000 && cut > 0 && cut < hits, "triangle hits %lld, cut %lld", hits, cut);
  CHECK(bad_bary == 0, "barycentrics rebuild the hit point (%lld bad)", bad_bary);
  CHECK(bad_nearest == 0, "nearest non-cut hit equals the reference (%lld bad)", bad_nearest);
  std::printf("alpha cut: %lld triangle hits, %lld cut, bary mismatches %lld, nearest mismatches %lld\n", hits, cut, bad_bary, bad_nearest);
}

// The blit (world_alpha_blit.cs_5_0.hlsl) in float: texel (x, y) of the slice samples the source at its centre
// ((x + 0.5) / 1024, (y + 0.5) / 256) with bilinear filtering and clamp, quantised as saturate(a) * 255 + 0.5,
// four texels per uint. Compared with a double-precision reference: at most 1 LSB apart.
static void TestBlitPacking() {
  std::mt19937 rng(11);
  std::uniform_real_distribution<float> unit(0.f, 1.f);
  int worst = 0;
  long long texels = 0;
  for (int scene = 0; scene < 12; ++scene) {
    const int width = 1 + static_cast<int>(rng() % 64u);
    const int height = 1 + static_cast<int>(rng() % 64u);
    std::vector<float> source(static_cast<size_t>(width) * height);
    for (float& a : source) a = unit(rng);
    auto texel_at = [&](int x, int y) {
      x = (std::max)(0, (std::min)(x, width - 1));
      y = (std::max)(0, (std::min)(y, height - 1));
      return source[static_cast<size_t>(y) * width + x];
    };
    for (uint32_t y = 0; y < falcom_world::kAlphaSliceTexelsY; ++y) {
      for (uint32_t word = 0; word < falcom_world::kAlphaSliceWords; ++word) {
        uint8_t gpu[4] = {};
        uint8_t ref[4] = {};
        for (uint32_t k = 0; k < 4u; ++k) {
          const uint32_t x = word * 4u + k;
          const float u = (static_cast<float>(x) + 0.5f) / 1024.f;
          const float v = (static_cast<float>(y) + 0.5f) / 256.f;
          const float fx = u * static_cast<float>(width) - 0.5f;
          const float fy = v * static_cast<float>(height) - 0.5f;
          const float ax = fx - std::floor(fx);
          const float ay = fy - std::floor(fy);
          const int x0 = static_cast<int>(std::floor(fx));
          const int y0 = static_cast<int>(std::floor(fy));
          const float a = (1.f - ax) * (1.f - ay) * texel_at(x0, y0) + ax * (1.f - ay) * texel_at(x0 + 1, y0)
                          + (1.f - ax) * ay * texel_at(x0, y0 + 1) + ax * ay * texel_at(x0 + 1, y0 + 1);
          gpu[k] = static_cast<uint8_t>((std::min)(std::max(a, 0.f), 1.f) * 255.f + 0.5f);
          const double dx = (static_cast<double>(x) + 0.5) / 1024.0 * width - 0.5;
          const double dy = (static_cast<double>(y) + 0.5) / 256.0 * height - 0.5;
          const double tx = dx - std::floor(dx);
          const double ty = dy - std::floor(dy);
          const int rx = static_cast<int>(std::floor(dx));
          const int ry = static_cast<int>(std::floor(dy));
          const double rd = (1.0 - tx) * (1.0 - ty) * texel_at(rx, ry) + tx * (1.0 - ty) * texel_at(rx + 1, ry)
                            + (1.0 - tx) * ty * texel_at(rx, ry + 1) + tx * ty * texel_at(rx + 1, ry + 1);
          ref[k] = static_cast<uint8_t>(std::floor((std::min)((std::max)(rd, 0.0), 1.0) * 255.0 + 0.5));
        }
        const uint32_t packed = falcom_world::PackAlphaTexels(gpu);
        for (uint32_t k = 0; k < 4u; ++k) {
          const int diff = std::abs(static_cast<int>((packed >> (k * 8u)) & 0xFFu) - static_cast<int>(ref[k]));
          worst = (std::max)(worst, diff);
          texels += 1;
        }
      }
    }
  }
  CHECK(worst <= 1, "blit packing within 1 LSB of the reference (worst %d over %lld texels)", worst, texels);
  std::printf("blit packing: worst %d LSB over %lld texels\n", worst, texels);

  // The TLAS descriptor carries the material slot (slice + 1) in header.z; the trace reads it there.
  bvh::LiveTlasInstance instance;
  instance.material = 5u;
  const bvh::WorldInstanceGPU descriptor = bvh::MakeLiveInstanceGPU(instance, 3u);
  CHECK(descriptor.header[0] == 3.f && descriptor.header[2] == 5.f, "descriptor header: mesh %f, material %f", descriptor.header[0], descriptor.header[2]);
  bvh::LiveTlasInstance plain;
  CHECK(bvh::MakeLiveInstanceGPU(plain, 1u).header[2] == 0.f, "no material: header.z 0");
}

// ---------------------------------------------------------------------------
// Phase 0 cache diagnostics (diagnostic only): tier golden case, TLAS diff, episodes, lighting gap, report
// lines, and the on/off check (the diagnostics change no TLAS, descriptor or revision).

static bvh::LiveTlasInstance TlasAt(uint64_t id, float x, bool wind = false) {
  bvh::LiveTlasInstance t;
  t.id = id;
  t.wind = wind;
  t.matrix[0] = t.matrix[5] = t.matrix[10] = 1.f;
  t.matrix[3] = x;
  t.bounds_min[0] = x - 1.f;
  t.bounds_max[0] = x + 1.f;
  t.bounds_min[1] = t.bounds_min[2] = -1.f;
  t.bounds_max[1] = t.bounds_max[2] = 1.f;
  return t;
}

static bool HasLine(const std::vector<std::string>& lines, const std::string& text) {
  return std::find(lines.begin(), lines.end(), text) != lines.end();
}

template <typename T>
static bool SameBytes(const std::vector<T>& a, const std::vector<T>& b) {
  return a.size() == b.size() && (a.empty() || std::memcmp(a.data(), b.data(), a.size() * sizeof(T)) == 0);
}

// Tier table of a fixed pool: region [-256, 256], camera at the origin.
static bvh::CacheTiers TierGolden() {
  std::vector<bvh::WorldMesh> meshes(3);
  meshes[0].uid = 10u;
  meshes[0].triangle_count = 100u;
  meshes[0].positions.resize(4);
  meshes[0].indices.resize(6);
  meshes[1].uid = 11u;
  meshes[1].triangle_count = 50u;
  meshes[1].positions.resize(3);
  meshes[1].indices.resize(3);
  meshes[1].uvs.resize(3);
  meshes[2].uid = 12u;
  meshes[2].triangle_count = 7u;
  const auto instance = [](uint32_t mesh, float x) {
    bvh::WorldInstance i;
    i.mesh_id = mesh;
    i.matrix[3] = x;
    i.bounds_min[0] = x - 1.f;
    i.bounds_max[0] = x + 1.f;
    i.bounds_min[1] = i.bounds_min[2] = -1.f;
    i.bounds_max[1] = i.bounds_max[2] = 1.f;
    return i;
  };
  const std::vector<bvh::WorldInstance> instances = {instance(0u, 50.f), instance(0u, 200.f), instance(1u, 300.f),
                                                     instance(1u, -120.f), instance(0u, -240.f)};
  bvh::PoolRegion region;
  for (int k = 0; k < 3; ++k) {
    region.min[k] = -256.f;
    region.max[k] = 256.f;
  }
  const float camera[3] = {};
  std::unordered_map<uint64_t, bvh::CacheResident> resident;
  resident[10u] = bvh::CacheResident{9u, 1000u};
  bvh::CacheTiers tiers;
  bvh::ComputeCacheTiers(instances, meshes, camera, region, resident, &tiers);
  return tiers;
}

static void TestCacheTiers() {
  const bvh::CacheTiers t = TierGolden();
  CHECK(t.self_check, "tier sums equal the pool totals");
  const bvh::CacheTier& t0 = t.tiers[0];
  CHECK(t0.instances == 2u && t0.instance_tris == 150u && t0.meshes == 2u && t0.pool_tris == 150u && t0.pool_bytes == 144u
            && t0.resident_meshes == 1u && t0.resident_tris == 9u && t0.resident_bytes == 1000u,
        "tier T0 (%llu, %llu, %llu)", (unsigned long long)t0.instances, (unsigned long long)t0.instance_tris, (unsigned long long)t0.meshes);
  const bvh::CacheTier& t1 = t.tiers[1];
  CHECK(t1.instances == 2u && t1.instance_tris == 200u && t1.meshes == 0u, "tier T1");
  const bvh::CacheTier& t2 = t.tiers[2];
  CHECK(t2.instances == 0u && t2.meshes == 0u, "tier T2 empty");
  const bvh::CacheTier& outside = t.tiers[3];
  CHECK(outside.instances == 1u && outside.instance_tris == 50u && outside.meshes == 0u, "outside region tier");
  const bvh::CacheTier& none = t.tiers[4];
  CHECK(none.instances == 0u && none.meshes == 1u && none.pool_tris == 7u && none.pool_bytes == 0u, "no instance tier (mesh once)");
  CHECK(t.instances == 5u && t.meshes == 3u && t.pool_tris == 157u && t.pool_bytes == 144u, "pool totals");
  CHECK(t.histogram[2] == 1u && t.histogram[4] == 1u && t.histogram[6] == 2u && t.histogram[7] == 1u, "histogram buckets");
  uint64_t histogram_total = 0u;
  for (const uint64_t count : t.histogram) histogram_total += count;
  CHECK(histogram_total == 5u, "histogram counts every instance (%llu)", (unsigned long long)histogram_total);
}

static void TestCacheDiff() {
  const std::vector<bvh::LiveTlasInstance> before = {TlasAt(1u, 10.f), TlasAt(2u, 20.f), TlasAt(3u, 300.f), TlasAt(4u, 50.f, true)};
  const std::vector<bvh::LiveTlasInstance> after = {TlasAt(5u, 100.f), TlasAt(3u, 301.f), TlasAt(2u, 20.f)};
  const float camera[3] = {};
  const bvh::CacheTlasDiff d = bvh::DiffTlasInstances(before, after, camera);
  CHECK(d.added == 1u && d.removed == 1u && d.moved == 1u, "diff: added %llu removed %llu moved %llu", (unsigned long long)d.added,
        (unsigned long long)d.removed, (unsigned long long)d.moved);
  CHECK(std::fabs(d.nearest_add_remove - 9.f) < 1e-4f, "nearest add/remove (%.3f)", (double)d.nearest_add_remove);
  CHECK(std::fabs(d.nearest_changed - 300.f) < 1e-4f, "nearest changed (%.3f)", (double)d.nearest_changed);
  const bvh::CacheTlasDiff none = bvh::DiffTlasInstances({}, {}, camera);
  CHECK(none.added == 0u && none.removed == 0u && none.moved == 0u && none.nearest_add_remove < 0.f && none.nearest_changed < 0.f, "empty diff");
  const bvh::CacheTlasDiff wind = bvh::DiffTlasInstances({TlasAt(7u, 5.f, true)}, {}, camera);
  CHECK(wind.removed == 0u && wind.nearest_add_remove < 0.f, "wind instance excluded from the diff");
}

static void TestCacheEpisodes(Device* dev) {
  const auto now = std::chrono::steady_clock::now();
  bvh::CacheEpisode e;
  e.open = true;
  e.start_frame = 100u;
  e.start_time = now;
  const uint32_t frames[6] = {105u, 110u, 120u, 130u, 140u, 150u};
  const uint64_t fresh[6] = {0u, 2u, 5u, 9u, 10u, 10u};
  for (int i = 0; i < 6; ++i) {
    bvh::CacheSample& s = e.samples.emplace_back();
    s.frame = frames[i];
    s.ms = static_cast<float>(i);
    s.tlas_fresh = fresh[i];
  }
  bvh::FinishCacheEpisode(&e, "settled");
  CHECK(!e.open && std::strcmp(e.state, "settled") == 0, "episode settled");
  CHECK(e.t80_valid && e.t80_frames == 30u && e.final_fresh == 10u, "t80 at +30 frames (%u, final %llu)", e.t80_frames, (unsigned long long)e.final_fresh);
  CHECK(e.t80_ms == 3.f, "t80 ms from the sample (%.1f)", (double)e.t80_ms);
  bvh::CacheEpisode interrupted;
  interrupted.open = true;
  bvh::FinishCacheEpisode(&interrupted, "interrupted");
  CHECK(!interrupted.open && std::strcmp(interrupted.state, "interrupted") == 0 && !interrupted.t80_valid, "interrupted episode has no t80");

  // Stale start: a rebuild before the episode start is not sampled; one at or after it is.
  bvh::ResetCacheDiagCounters();
  {
    std::lock_guard<std::mutex> lock(bvh::g_cache_diag.mutex);
    bvh::g_cache_diag.data.episode.open = true;
    bvh::g_cache_diag.data.episode.start_frame = 100u;
    bvh::g_cache_diag.data.episode.start_time = now;
  }
  const std::vector<bvh::LiveTlasInstance> tlas = {TlasAt(1u, 10.f)};
  bvh::RecordCacheTlasRebuild({}, tlas, true, 50u, 1.f);
  bvh::RecordCacheTlasRebuild({}, tlas, false, 120u, 2.f);
  bvh::RecordCacheTlasRebuild({}, tlas, true, 120u, 4.f);
  {
    std::lock_guard<std::mutex> lock(bvh::g_cache_diag.mutex);
    CHECK(bvh::g_cache_diag.data.episode.samples.size() == 1u && bvh::g_cache_diag.data.episode.samples[0].frame == 120u,
          "stale rebuild not credited, failed build not counted (%zu samples)", bvh::g_cache_diag.data.episode.samples.size());
    CHECK(bvh::g_cache_diag.data.work.tlas_ms == 5.f && bvh::g_cache_diag.data.work.tlas_rebuilds == 2u
              && bvh::g_cache_diag.data.episode.work.tlas_ms == 4.f && bvh::g_cache_diag.data.episode.work.tlas_rebuilds == 1u,
          "TLAS ms since reset (%.2f, %llu) and episode (%.2f, %llu) count only replaced builds, the stale one at its time",
          (double)bvh::g_cache_diag.data.work.tlas_ms, (unsigned long long)bvh::g_cache_diag.data.work.tlas_rebuilds,
          (double)bvh::g_cache_diag.data.episode.work.tlas_ms, (unsigned long long)bvh::g_cache_diag.data.episode.work.tlas_rebuilds);
    CHECK(bvh::g_cache_diag.data.tlas.rebuilds == 2u && bvh::g_cache_diag.data.tlas.added == 2u, "two replaced TLAS builds counted, the failed one not");
    CHECK(bvh::g_cache_diag.data.episode.quiet_presents == 0u, "a rebuild restarts the settle count");
  }
  bvh::ResetCacheDiagCounters();

  // Lighting gap and capture-off reset (UpdateCacheDiag with the switch off).
  bvh::CacheLighting lighting;
  const bool seq[7] = {false, true, true, false, false, false, true};
  for (const bool dark : seq) bvh::AddCacheLightingSample(&lighting, dark);
  CHECK(lighting.presents == 7u && lighting.dark_presents == 3u && lighting.dark_runs == 2u && lighting.longest_dark == 2u
            && lighting.longest_lit == 3u && lighting.dark,
        "lighting runs (dark %llu of %llu, runs %llu, longest dark %llu, lit %llu)", (unsigned long long)lighting.dark_presents,
        (unsigned long long)lighting.presents, (unsigned long long)lighting.dark_runs, (unsigned long long)lighting.longest_dark,
        (unsigned long long)lighting.longest_lit);
  {
    std::lock_guard<std::mutex> lock(bvh::g_cache_diag.mutex);
    bvh::g_cache_diag.data.lighting = lighting;
  }
  bvh::g_cache_diag.enabled.store(false);
  bvh::UpdateCacheDiag(dev);
  {
    std::lock_guard<std::mutex> lock(bvh::g_cache_diag.mutex);
    CHECK(bvh::g_cache_diag.data.lighting.presents == 0u, "capture off resets the lighting gap (%llu)", (unsigned long long)bvh::g_cache_diag.data.lighting.presents);
  }
  bvh::g_cache_diag.enabled.store(true);
  // Dark rule: equal recent stamps are lit, stamps 5 frames old are dark, stamps 1 frame ahead of frame are lit (no unsigned wrap).
  {
    auto& frame = falcom_world::g_state.frame;
    falcom_world::g_state.depth_source.valid = true;
    falcom_world::g_state.camera.valid = true;
    frame.store(7000u);
    falcom_world::g_state.depth_source.frame = 6999u;
    falcom_world::g_state.camera.frame = 6999u;
    bvh::UpdateCacheDiag(dev);
    CHECK(!bvh::g_cache_diag.data.lighting.dark, "equal recent stamps are lit");
    falcom_world::g_state.depth_source.frame = 6995u;
    falcom_world::g_state.camera.frame = 6995u;
    bvh::UpdateCacheDiag(dev);
    CHECK(bvh::g_cache_diag.data.lighting.dark, "stamps 5 frames old are dark");
    falcom_world::g_state.depth_source.frame = 7001u;
    falcom_world::g_state.camera.frame = 7001u;
    bvh::UpdateCacheDiag(dev);
    CHECK(!bvh::g_cache_diag.data.lighting.dark, "stamps 1 frame ahead are lit, not wrapped to dark");
    {
      std::lock_guard<std::mutex> lock(bvh::g_cache_diag.mutex);
      bvh::g_cache_diag.data.presents = 0u;
    }
  }
  bvh::ResetCacheDiagCounters();
}

// A Dump request with collection off: the present computes the tiers, counts no present, and clears the request.
static void TestCacheForcedDump(Device* dev) {
  bvh::ResetCacheDiagCounters();
  bvh::g_cache_diag.enabled.store(false);
  bvh::g_cache_diag.force_tiers_requested.store(true);
  bvh::UpdateCacheDiag(dev);
  {
    std::lock_guard<std::mutex> lock(bvh::g_cache_diag.mutex);
    CHECK(bvh::g_cache_diag.data.tiers_valid, "forced dump with collection off keeps the tiers");
    CHECK(bvh::g_cache_diag.data.presents == 0u, "collection off: no present counted (%llu)", (unsigned long long)bvh::g_cache_diag.data.presents);
  }
  CHECK(!bvh::g_cache_diag.force_tiers_requested.load(), "dump request consumed by the present");
  bvh::g_cache_diag.enabled.store(true);
  bvh::ResetCacheDiagCounters();
}

static void TestCacheBucket() {
  const float edges[3] = {1.f, 2.f, 4.f};
  CHECK(bvh::CacheBucket(0.f, edges, 3u) == 0u, "value 0 goes to bucket 0");
  CHECK(bvh::CacheBucket(1.f, edges, 3u) == 1u, "an edge value goes to the upper bucket");
  CHECK(bvh::CacheBucket(std::nextafter(1.f, 0.f), edges, 3u) == 0u, "a value just below an edge stays in the lower bucket");
  CHECK(bvh::CacheBucket(1e30f, edges, 3u) == 3u, "a huge value goes to the last bucket");
}

static bvh::WorldMesh ResightMesh(uint64_t signature, uint32_t triangles, float size) {
  bvh::WorldMesh mesh;
  mesh.signature = signature;
  mesh.triangle_count = triangles;
  mesh.positions.resize(8u);
  mesh.verified = true;
  for (int k = 0; k < 3; ++k) {
    mesh.bbox_min[k] = -size;
    mesh.bbox_max[k] = size;
  }
  return mesh;
}

static void ResetResight(bvh::PoolResightTracker* tracker) {
  tracker->counters = bvh::PoolResightCounters{};
  tracker->by_signature.clear();
  tracker->shape_count.clear();
  tracker->sweep_frame = 0u;
  tracker->enabled.store(true);
}

static void TestPoolResightTracker() {
  std::lock_guard lock(bvh::g_pool.mutex);
  bvh::PoolResightTracker& tracker = bvh::g_pool.resight;
  ResetResight(&tracker);
  tracker.Retire(ResightMesh(0x7001u, 12u, 1.f), 10u);
  tracker.Sight(ResightMesh(0x7001u, 13u, 1.f), 20u, 0u);
  CHECK(tracker.counters.resights == 1u && tracker.counters.count_mismatch == 1u, "same signature, other triangle count: count mismatch");
  ResetResight(&tracker);
  tracker.Retire(ResightMesh(0x7002u, 12u, 1.f), 10u);
  tracker.Sight(ResightMesh(0x7003u, 12u, 1.004f), 20u, 0u);
  CHECK(tracker.counters.drift_near_misses == 1u && tracker.counters.resights == 0u, "bounds 1.000 vs 1.004, equal counts: drift near miss");
  ResetResight(&tracker);
  tracker.Retire(ResightMesh(0x7004u, 12u, 1.f), 10u);
  tracker.Sight(ResightMesh(0x7005u, 12u, 1.02f), 20u, 0u);
  CHECK(tracker.counters.drift_near_misses == 0u, "bounds 1.00 vs 1.02 are not a drift near miss");
  ResetResight(&tracker);
  tracker.enabled.store(false);
  tracker.Retire(ResightMesh(0x7006u, 12u, 1.f), 10u);
  tracker.Sight(ResightMesh(0x7006u, 12u, 1.f), 20u, 0u);
  CHECK(tracker.counters.retired == 0u && tracker.counters.resights == 0u && tracker.counters.new_meshes == 0u, "disabled tracker counts nothing");
  ResetResight(&tracker);
  for (size_t k = 0; k < bvh::kPoolResightMax; ++k) tracker.Retire(ResightMesh(0x100000u + k, 12u, 1.f), 1000u);
  tracker.Retire(ResightMesh(0x9000u, 12u, 1.f), 1000u);
  CHECK(tracker.counters.retired_dropped == 1u, "a retirement at the cap is dropped (%llu)", (unsigned long long)tracker.counters.retired_dropped);
  tracker.Retire(ResightMesh(0x9000u, 12u, 1.f), 1000u + bvh::kPoolResightHorizon + 1u);
  CHECK(tracker.counters.expired == bvh::kPoolResightMax && tracker.by_signature.size() == 1u,
        "retirements older than the horizon expire and the insert succeeds (%llu expired)", (unsigned long long)tracker.counters.expired);
  ResetResight(&tracker);
  const uint32_t gaps[10] = {0u, 119u, 120u, 599u, 600u, 1799u, 1800u, 3599u, 3600u, 100000u};
  for (size_t k = 0; k < 10; ++k) {
    tracker.Retire(ResightMesh(0x8000u + k, 12u, 1.f), 1000u);
    tracker.Sight(ResightMesh(0x8000u + k, 12u, 1.f), 1000u + gaps[k], 0u);
  }
  const auto& g = tracker.counters.gap;
  CHECK(g[0] == 2u && g[1] == 2u && g[2] == 2u && g[3] == 2u && g[4] == 2u && tracker.counters.resights == 10u,
        "gap buckets {0,0,1,1,2,2,3,3,4,4} (%llu %llu %llu %llu %llu)", (unsigned long long)g[0], (unsigned long long)g[1],
        (unsigned long long)g[2], (unsigned long long)g[3], (unsigned long long)g[4]);
  ResetResight(&tracker);
}

static void TestCacheResight() {
  bvh::ResetWorldPool();
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    ResetResight(&bvh::g_pool.resight);
  }
  auto& frame = falcom_world::g_state.frame;
  frame.store(30000u);
  AddMesh(0x61, BoxMesh(1.f));
  RetireMesh(0x61);
  frame.fetch_add(50u);
  AddMesh(0x62, BoxMesh(1.f));
  std::lock_guard lock(bvh::g_pool.mutex);
  const bvh::PoolResightCounters& c = bvh::g_pool.resight.counters;
  CHECK(c.retired == 1u && c.resights == 1u && c.revivable_tris == 12u && c.count_mismatch == 0u,
        "re-sighting after retirement: retired %llu, resights %llu, revivable tris %llu, count mismatch %llu", (unsigned long long)c.retired,
        (unsigned long long)c.resights, (unsigned long long)c.revivable_tris, (unsigned long long)c.count_mismatch);
  CHECK(c.gap[0] == 1u && c.new_meshes == 2u && bvh::g_pool.resight.by_signature.empty(),
        "gap of 50 presents in bucket 0, two new meshes, the re-sighted entry erased");
}

static void TestCacheTierBuckets() {
  // TierGolden: meshes have no bbox (extent 0 -> bucket 0). Instance sides are 2 m; ratio = 2 / max(distance, 1).
  // Distances: 49 (T0), 119 (T0), 199 (T1), 239 (T1), 299 (outside). Ratios 0.0408, 0.0168, 0.01005, 0.00837, 0.00669.
  const bvh::CacheTiers t = TierGolden();
  const bvh::CacheTier& t0 = t.tiers[0];
  CHECK(t0.extent[0] == 2u && t0.ratio[3] == 1u && t0.ratio[4] == 1u && t0.ratio_tris[3] == 50u && t0.ratio_tris[4] == 100u,
        "T0 extent and ratio buckets (%llu, %llu, %llu, %llu, %llu)", (unsigned long long)t0.extent[0], (unsigned long long)t0.ratio[3],
        (unsigned long long)t0.ratio[4], (unsigned long long)t0.ratio_tris[3], (unsigned long long)t0.ratio_tris[4]);
  const bvh::CacheTier& t1 = t.tiers[1];
  CHECK(t1.ratio[2] == 1u && t1.ratio[3] == 1u && t1.ratio_tris[2] == 100u && t1.ratio_tris[3] == 100u, "T1 ratio buckets");
  CHECK(t.tiers[3].ratio[2] == 1u && t.tiers[3].ratio_tris[2] == 50u, "outside tier ratio bucket");
  CHECK(t.tiers[4].extent[0] == 1u, "no-instance tier: its mesh counted once in extent");
  bool histogram_ok = true;
  for (const bool check : t.hist_check) histogram_ok = histogram_ok && check;
  CHECK(histogram_ok, "hist_check: extent sums = meshes, ratio sums = instances, ratio_tris sums = instance tris, every tier");
  // Ratio exactly 0.005 at 200 m (side 1 m): an edge value goes to the upper bucket, bucket 2.
  std::vector<bvh::WorldMesh> meshes(1);
  meshes[0].triangle_count = 1u;
  bvh::WorldInstance edge;
  edge.mesh_id = 0u;
  edge.bounds_min[0] = 200.f;
  edge.bounds_max[0] = 201.f;
  edge.bounds_min[1] = edge.bounds_min[2] = -0.5f;
  edge.bounds_max[1] = edge.bounds_max[2] = 0.5f;
  bvh::PoolRegion region;
  for (int k = 0; k < 3; ++k) {
    region.min[k] = -256.f;
    region.max[k] = 256.f;
  }
  const float camera[3] = {};
  bvh::CacheTiers at_edge;
  bvh::ComputeCacheTiers({edge}, meshes, camera, region, {}, &at_edge);
  CHECK(at_edge.tiers[1].ratio[2] == 1u && at_edge.tiers[1].ratio_tris[2] == 1u && at_edge.hist_check[1],
        "ratio 1/200 = 0.005 at 200 m goes to bucket 2");
}

static void TestCacheEpisodeWork() {
  bvh::CacheEpisode e;
  e.open = true;
  e.start_frame = 100u;
  const uint32_t frames[3] = {105u, 110u, 120u};
  const uint64_t fresh[3] = {0u, 8u, 10u};
  for (int i = 0; i < 3; ++i) {
    bvh::CacheSample& s = e.samples.emplace_back();
    s.frame = frames[i];
    s.tlas_fresh = fresh[i];
    s.work.blas_ms = static_cast<float>(i + 1);
  }
  bvh::FinishCacheEpisode(&e, "settled");
  CHECK(e.t80_valid && e.t80_frames == 10u && e.t80_work.blas_ms == 2.f,
        "t80 work is the work of the t80 sample (frames %u, blas %.1f)", e.t80_frames, (double)e.t80_work.blas_ms);
}

static void TestCacheCaptureTime(Device* dev) {
  bvh::ResetCacheDiagCounters();
  bvh::g_cache_diag.enabled.store(true);
  bvh::g_pool.cpu_capture_us.store(2500u);
  bvh::UpdateCacheDiag(dev);
  {
    std::lock_guard<std::mutex> lock(bvh::g_cache_diag.mutex);
    CHECK(bvh::g_cache_diag.data.work.capture_ms == 2.5f && bvh::g_pool.cpu_capture_us.load() == 0u,
          "2500 us of capture CPU gives 2.5 ms and clears the counter (%.3f ms)", (double)bvh::g_cache_diag.data.work.capture_ms);
  }
  bvh::ResetCacheDiagCounters();
}

// Reads a dump file back as text.
static std::string ReadDumpText(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

static void TestCacheDiagJson(Device* dev) {
  bvh::ResetCacheDiagCounters();
  const std::filesystem::path path = bvh::WriteCacheDiagFile(bvh::GetBvhDeviceData(dev), 1234u, "", bvh::CacheSwitches{});
  const std::string text = ReadDumpText(path);
  CHECK(JsonError(text).empty(), "cache_diag file is valid JSON: %s", JsonError(text).c_str());
  const char* keys[] = {"\"schema\": 4", "\"resight\": {", "\"gap\": [", "\"capture\": {", "\"extent_buckets\": [",
                        "\"ratio_buckets\": [", "\"ratio_tris\": [", "\"hist_check\": ", "\"ms_sum\": ", "\"t80_work\": ", "\"work\": ",
                        "\"phase_ms\": {", "\"noop\": {", "\"reasons\": {", "\"entry_latency\": {", "\"label\": \"\"", "\"label_raw\": \"\"",
                        "\"timestamp\": \"", "\"presents\": ", "\"tiers_valid\": ", "\"tiers_frame\": ", "\"switches\": {", "\"slack\": {",
                        "\"gpu\": {", "\"gpu_check\": {", "\"live\": {", "\"errors\": {"};

  for (const char* key : keys) CHECK(text.find(key) != std::string::npos, "json has %s", key);
  const bvh::LiveBvhStats& live = bvh::GetBvhDeviceData(dev)->live;
  CHECK(text.find("\"live\": {\"tlas_instances\": " + std::to_string(live.tlas_instances) + ", \"tlas_rebuilds\": " + std::to_string(live.tlas_rebuilds)) != std::string::npos,
        "live block equals data->live");
  CHECK(text.find("\"tlas_frame\": " + std::to_string(live.tlas_frame) + ", \"refit_lag_last\": " + std::to_string(live.refit_lag_last)
                  + ", \"refit_lag_max\": " + std::to_string(live.refit_lag_max) + "}") != std::string::npos,
        "live block has tlas_frame and refit lag");
  bvh::ResetCacheDiagCounters();
}

static void TestDiagLabel() {
  CHECK(bvh::SanitizeDiagLabel("a b/c") == "a_b_c", "space and slash become _ (%s)", bvh::SanitizeDiagLabel("a b/c").c_str());
  CHECK(bvh::SanitizeDiagLabel("..\\x:*?") == "x", "path characters removed (%s)", bvh::SanitizeDiagLabel("..\\x:*?").c_str());
  CHECK(bvh::SanitizeDiagLabel("").empty(), "empty label stays empty");
  CHECK(bvh::SanitizeDiagLabel(std::string(100, 'a')).size() == 40u, "label truncated to 40 bytes (%zu)", bvh::SanitizeDiagLabel(std::string(100, 'a')).size());
  CHECK(bvh::SanitizeDiagLabel("a\xC3\xA9" "b") == "a_b", "non-ASCII bytes collapse to one _ (%s)", bvh::SanitizeDiagLabel("a\xC3\xA9" "b").c_str());
  CHECK(bvh::SanitizeDiagLabel("__x--") == "x", "leading and trailing _ . - trimmed (%s)", bvh::SanitizeDiagLabel("__x--").c_str());
  CHECK(bvh::SanitizeDiagLabel("A-b.c_9") == "A-b.c_9", "allowed set kept");
}

// Two dumps in the same second with the same label: two distinct files, each keeping its own frame; no fixed-name file.
static void TestDiagSeparateFiles(Device* dev) {
  bvh::ResetCacheDiagCounters();
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(dev);
  const bvh::CacheSwitches off{};
  std::error_code ec;
  std::filesystem::remove(bvh::PoolOutputDir() / "world_cache_diag.json", ec);  // a file left by an older build
  const std::filesystem::path first = bvh::WriteCacheDiagFile(data, 1u, "same", off);
  const std::filesystem::path second = bvh::WriteCacheDiagFile(data, 2u, "same", off);
  const std::filesystem::path other = bvh::WriteCacheDiagFile(data, 3u, "other", off);
  CHECK(first != second && first != other && second != other, "dumps get distinct paths (%s / %s / %s)", first.filename().string().c_str(),
        second.filename().string().c_str(), other.filename().string().c_str());
  CHECK(std::filesystem::exists(first) && std::filesystem::exists(second) && std::filesystem::exists(other), "every dump file exists");
  CHECK(ReadDumpText(first).find("\"frame\": 1,") != std::string::npos, "first dump keeps frame 1");
  CHECK(ReadDumpText(second).find("\"frame\": 2,") != std::string::npos, "second dump keeps frame 2");
  CHECK(first.parent_path().filename() == "diag", "dumps are written in the diag folder");
  CHECK(!std::filesystem::exists(bvh::PoolOutputDir() / "world_cache_diag.json"), "no world_cache_diag.json is written");
  CHECK(bvh::DiagFilePath("perf", "same", std::chrono::system_clock::now()).filename().string().rfind("perf_", 0) == 0,
        "perf files use their own prefix");
  bvh::ResetCacheDiagCounters();
}

// The four switches are written exactly as they were passed in.
static void TestDiagSwitches(Device* dev) {
  bvh::ResetCacheDiagCounters();
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(dev);
  const bvh::CacheSwitches a{true, false, true, false};
  const bvh::CacheSwitches b{false, true, false, true};
  const std::string text_a = ReadDumpText(bvh::WriteCacheDiagFile(data, 5u, "", a));
  const std::string text_b = ReadDumpText(bvh::WriteCacheDiagFile(data, 6u, "", b));
  CHECK(text_a.find("\"switches\": {\"collect_cache_diagnostics\": true, \"skip_noop_tlas\": false, \"slack_tlas\": true, \"live_bvh\": false}") != std::string::npos,
        "switch combination A written exactly");
  CHECK(text_b.find("\"switches\": {\"collect_cache_diagnostics\": false, \"skip_noop_tlas\": true, \"slack_tlas\": false, \"live_bvh\": true}") != std::string::npos,
        "switch combination B written exactly");
  bvh::ResetCacheDiagCounters();
}

// GPU check, trace timer, error counters: each field reaches the dump; errors.total is the sum of the groups.
static void TestDiagGpuAndErrors(Device* dev) {
  bvh::ResetCacheDiagCounters();
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(dev);
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    bvh::g_pool.stats.staging_failures = 3u;
  }
  {
    std::lock_guard<std::mutex> lock(bvh::g_deform_live.mutex);
    bvh::g_deform_live.stats.failed = 1u;
  }
  data->live.blas_invalid = 2u;
  data->live.tlas_invalid = 0u;
  data->trace_stats.invalid_refs = 4u;
  data->live.gpu_checked = true;
  data->live.gpu_ok = false;
  data->live.gpu_result = "MISMATCH test";
  data->live.gpu_check_frame = 77u;
  data->trace_timer.samples = 2u;
  data->trace_timer.ms_sum = 5.0;
  data->trace_timer.ms_max = 3.f;
  data->trace_timer.last_ms = 2.f;
  const std::string text = ReadDumpText(bvh::WriteCacheDiagFile(data, 9u, "", bvh::CacheSwitches{}));
  const bvh::CacheErrors e = bvh::CollectCacheErrors(data);
  const uint64_t sum = e.meshes + e.readback.map_failures + e.readback.staging_failures + e.readback.base_mismatch + e.bvh.blas_invalid
                       + e.bvh.tlas_invalid + e.bvh.upload_failures + e.bvh.refit_failures + e.bvh.invalid_refs + e.bvh.stack_overflow
                       + e.deform.failed + e.deform.resource_failures + e.deform.map_failures;
  CHECK(e.total == sum && e.total >= 10u, "errors.total is the sum of its groups (%llu vs %llu)", (unsigned long long)e.total, (unsigned long long)sum);
  const std::string total = "\"errors\": {\"total\": " + std::to_string(e.total) + ",";
  CHECK(text.find(total) != std::string::npos, "dump errors.total equals the live sum (%llu)", (unsigned long long)e.total);
  CHECK(text.find("\"staging_failures\": 3") != std::string::npos && text.find("\"invalid_refs\": 4") != std::string::npos
            && text.find("\"failed\": 1,") != std::string::npos,
        "error fields reach the dump");
  CHECK(text.find("\"avg_ms\": 2.5") != std::string::npos && text.find("\"samples\": 2") != std::string::npos
            && text.find("\"last_ms\": 2") != std::string::npos,
        "trace average is sum / samples");
  CHECK(text.find("\"gpu_check\": {\"checked\": true, \"ok\": false, \"frame\": 77, \"result\": \"MISMATCH test\"}") != std::string::npos,
        "gpu check fields reach the dump");
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    bvh::g_pool.stats.staging_failures = 0u;
  }
  {
    std::lock_guard<std::mutex> lock(bvh::g_deform_live.mutex);
    bvh::g_deform_live.stats.failed = 0u;
  }
  data->live.blas_invalid = 0u;
  data->trace_stats.invalid_refs = 0u;
  data->live.gpu_checked = false;
  data->trace_timer = bvh::GpuTimer{};
  bvh::ResetCacheDiagCounters();
}

// Performance test helpers. The window test drives UpdatePerfRun with a synthetic 16 ms clock.
static std::string PerfResult() {
  std::lock_guard<std::mutex> lock(bvh::g_cache_diag.mutex);
  return bvh::g_cache_diag.data.last_result;
}

static std::string WrittenPath(const std::string& result) {
  const size_t begin = result.find("wrote ");
  if (begin == std::string::npos) return "";
  const size_t end = result.find(" (", begin);
  return result.substr(begin + 6u, end - begin - 6u);
}

static size_t CountOf(const std::string& text, const char* needle) {
  size_t count = 0u;
  for (size_t p = text.find(needle); p != std::string::npos; p = text.find(needle, p + 1u)) ++count;
  return count;
}

// The counters one present adds: rebuild 0.5 ms, refit 0.25, skip 0.125, apply 0.25, BLAS 0.5, GPU trace 2.0 ms per sample, hooks 0.25 ms.
static void PerfPresentCosts(bvh::BvhDeviceData* data) {
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    bvh::g_pool.tlas_timing.rebuild_ms_sum += 0.5f;
    bvh::g_pool.tlas_timing.rebuilds += 1u;
    bvh::g_pool.tlas_timing.refit_ms_sum += 0.25f;
    bvh::g_pool.tlas_timing.refits += 1u;
  }
  data->live.tlas_skip_ms_sum += 0.125f;
  data->live.tlas_skips += 1u;
  data->live.slack_apply_ms_sum += 0.25f;
  data->live.slack_applies += 1u;
  data->live.blas_build_ms_sum += 0.5f;
  data->live.blas_builds += 1u;
  data->trace_timer.ms_sum += 2.0;
  data->trace_timer.samples += 1u;
  bvh::g_pool.cpu_hook_frame_us = 250u;
}

// Refused while active, while a map change is swept, and during the refill; allowed 400 presents after the episode start.
static void TestPerfRefusals(Device* dev) {
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(dev);
  const bvh::CacheSwitches sw{};
  const auto base = std::chrono::steady_clock::now();
  uint32_t saved_start = 0u;
  uint32_t saved_map = 0u;
  uint32_t saved_serial = 0u;
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    saved_start = bvh::g_pool.episode_start_frame;
    saved_map = bvh::g_pool.map_change_frame;
    saved_serial = bvh::g_pool.episode_serial;
  }
  bvh::g_pool.episode_start_frame = 4990u;  // 10 presents after the episode start
  bvh::g_pool.map_change_frame = 0u;
  bvh::RequestPerfRun("a");
  bvh::UpdatePerfRun(data, 5000u, 0u, base, sw);
  CHECK(!bvh::g_perf.active.load() && PerfResult().find("refused: the pool is refilling") != std::string::npos,
        "refused during the refill (%s)", PerfResult().c_str());
  bvh::g_pool.episode_start_frame = 0u;
  bvh::g_pool.map_change_frame = 4000u;
  bvh::RequestPerfRun("b");
  bvh::UpdatePerfRun(data, 5000u, 0u, base, sw);
  CHECK(!bvh::g_perf.active.load() && PerfResult().find("refused: a map change is being swept") != std::string::npos,
        "refused while a map change is swept (%s)", PerfResult().c_str());
  bvh::g_pool.episode_start_frame = 4600u;  // 400 presents after the episode start
  bvh::g_pool.map_change_frame = 0u;
  bvh::RequestPerfRun("c");
  bvh::UpdatePerfRun(data, 5000u, 0u, base, sw);
  CHECK(bvh::g_perf.active.load() && PerfResult().find("started") != std::string::npos, "allowed 400 presents after the episode start");
  const uint32_t start_frame = bvh::g_perf.run.start_frame;
  bvh::RequestPerfRun("d");
  bvh::UpdatePerfRun(data, 5001u, 0u, base + std::chrono::milliseconds(16), sw);
  CHECK(bvh::g_perf.active.load() && bvh::g_perf.run.start_frame == start_frame && PerfResult().find("already active") != std::string::npos,
        "refused while active, the run is unchanged (%s)", PerfResult().c_str());
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    bvh::g_pool.episode_serial += 1u;
  }
  bvh::UpdatePerfRun(data, 5002u, 0u, base + std::chrono::milliseconds(32), sw);
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    bvh::g_pool.episode_serial = saved_serial;
    bvh::g_pool.episode_start_frame = saved_start;
    bvh::g_pool.map_change_frame = saved_map;
  }
  CHECK(!bvh::g_perf.active.load(), "the run ended");
  bvh::ResetCacheDiagCounters();
}

// 30 s of presents at 16 ms: the window ends at the first present with elapsed >= 30 s (1875 intervals).
static std::string RunPerfWindow(bvh::BvhDeviceData* data, uint32_t first_frame, bool collect, int* presents) {
  const auto base = std::chrono::steady_clock::now();
  const bvh::CacheSwitches sw{collect, false, false, true};
  bvh::g_cache_diag.enabled.store(collect);
  bvh::RequestPerfRun("window");
  bvh::UpdatePerfRun(data, first_frame, 0u, base, sw);
  int k = 0;
  while (bvh::g_perf.active.load() && k < 5000) {
    ++k;
    PerfPresentCosts(data);
    bvh::UpdatePerfRun(data, first_frame + static_cast<uint32_t>(k), 0u, base + std::chrono::milliseconds(16 * k), sw);
  }
  *presents = k;
  bvh::g_cache_diag.enabled.store(true);
  return PerfResult();
}

static void TestPerfWindow(Device* dev) {
  bvh::ResetCacheDiagCounters();
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(dev);
  uint32_t saved_start = 0u;
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    saved_start = bvh::g_pool.episode_start_frame;
    bvh::g_pool.episode_start_frame = 0u;
  }
  int presents_off = 0;
  const std::string off = RunPerfWindow(data, 5000u, false, &presents_off);
  const std::string path_off = WrittenPath(off);
  int presents_on = 0;
  const std::string on = RunPerfWindow(data, 9000u, true, &presents_on);
  const std::string path_on = WrittenPath(on);
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    bvh::g_pool.episode_start_frame = saved_start;
  }
  CHECK(presents_off == 1875 && presents_on == 1875, "window ends at the first present with elapsed >= 30 s (%d and %d presents)", presents_off, presents_on);
  CHECK(!path_off.empty() && !path_on.empty(), "a perf file is written (%s)", off.c_str());
  for (const std::string& path : {path_off, path_on}) {
    const std::string text = ReadDumpText(path);
    CHECK(JsonError(text).empty(), "perf file is valid JSON: %s", JsonError(text).c_str());
    CHECK(text.find("\"status\": \"complete\"") != std::string::npos, "perf file status complete");
    CHECK(text.find("\"frame_time\": {\"avg\": {\"value\": 16,") != std::string::npos, "frame time average is 16 ms");
    CHECK(text.find("\"actual_s\": {\"value\": 30,") != std::string::npos && text.find("\"presents\": {\"value\": 1875,") != std::string::npos,
          "window and present count written");
    CHECK(text.find("\"tlas_rebuild\": {\"avg_per_present\": {\"value\": 0.5,") != std::string::npos, "rebuild average per present 0.5 ms");
    CHECK(text.find("\"tlas_skip\": {\"avg_per_present\": {\"value\": 0.125,") != std::string::npos, "skip average per present 0.125 ms");
    CHECK(text.find("\"tlas_apply\": {\"avg_per_present\": {\"value\": 0.25,") != std::string::npos, "apply average per present 0.25 ms");
    CHECK(text.find("\"tlas_refit\": {\"avg_per_present\": {\"value\": 0.25,") != std::string::npos, "refit average per present 0.25 ms");
    CHECK(text.find("\"blas_build\": {\"avg_per_present\": {\"value\": 0.5,") != std::string::npos, "BLAS average per present 0.5 ms");
    CHECK(text.find("\"pool_hooks\": {\"avg_per_present\": {\"value\": 0.25,") != std::string::npos, "hooks average per present 0.25 ms");
    CHECK(text.find("\"gpu_trace\": {\"basis\": \"measured\", \"avg_per_measured_frame\": {\"value\": 2,") != std::string::npos,
          "GPU trace average per measured frame 2 ms");
    CHECK(CountOf(text, "\"value\": ") == CountOf(text, "\"unit\": "), "every value object has a unit");
  }
  CHECK(path_off != path_on, "the two runs write two files");
  bvh::ResetCacheDiagCounters();
}

// Aborted by a map change or pool reset during the run: the file is written with the partial averages.
static void TestPerfAbort(Device* dev) {
  bvh::ResetCacheDiagCounters();
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(dev);
  const auto base = std::chrono::steady_clock::now();
  const bvh::CacheSwitches sw{};
  uint32_t saved_start = 0u;
  uint32_t saved_serial = 0u;
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    saved_start = bvh::g_pool.episode_start_frame;
    saved_serial = bvh::g_pool.episode_serial;
    bvh::g_pool.episode_start_frame = 0u;
  }
  bvh::RequestPerfRun("abort");
  bvh::UpdatePerfRun(data, 5000u, 0u, base, sw);
  for (int k = 1; k <= 3; ++k) {
    PerfPresentCosts(data);
    bvh::UpdatePerfRun(data, 5000u + static_cast<uint32_t>(k), 0u, base + std::chrono::milliseconds(16 * k), sw);
  }
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    bvh::g_pool.episode_serial += 1u;
  }
  bvh::UpdatePerfRun(data, 5004u, 0u, base + std::chrono::milliseconds(64), sw);
  const std::string result = PerfResult();
  const std::string text = ReadDumpText(WrittenPath(result));
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    bvh::g_pool.episode_serial = saved_serial;
    bvh::g_pool.episode_start_frame = saved_start;
  }
  CHECK(!bvh::g_perf.active.load() && result.find("aborted (map change or pool reset at f5004)") != std::string::npos, "abort ends the run (%s)", result.c_str());
  CHECK(text.find("\"status\": \"aborted\"") != std::string::npos && text.find("\"abort_reason\": \"map change or pool reset at f5004\"") != std::string::npos,
        "aborted file carries the reason");
  CHECK(text.find("\"presents\": {\"value\": 3,") != std::string::npos, "partial averages: 3 presents");
  bvh::ResetCacheDiagCounters();
}

// A switch changed during the run: stable false. A reset request while the run is active does not touch the BLAS counters.
static void TestPerfSwitchesStable(Device* dev) {
  bvh::ResetCacheDiagCounters();
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(dev);
  const auto base = std::chrono::steady_clock::now();
  uint32_t saved_start = 0u;
  uint32_t saved_serial = 0u;
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    saved_start = bvh::g_pool.episode_start_frame;
    saved_serial = bvh::g_pool.episode_serial;
    bvh::g_pool.episode_start_frame = 0u;
  }
  const bvh::CacheSwitches off{};
  const bvh::CacheSwitches slack{false, false, true, false};
  bvh::RequestPerfRun("switch");
  bvh::UpdatePerfRun(data, 5000u, 0u, base, off);
  PerfPresentCosts(data);
  bvh::UpdatePerfRun(data, 5001u, 0u, base + std::chrono::milliseconds(16), slack);
  PerfPresentCosts(data);
  data->live.blas_builds = 7u;
  bvh::g_cache_diag.reset_requested.store(true);
  bvh::UpdateCacheDiag(dev);
  CHECK(data->live.blas_builds == 7u, "reset ignored while a run is active (builds %llu)", (unsigned long long)data->live.blas_builds);
  CHECK(PerfResult().find("Reset ignored") != std::string::npos, "reset ignored: result line says so");
  bvh::UpdatePerfRun(data, 5002u, 0u, base + std::chrono::milliseconds(32), off);
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    bvh::g_pool.episode_serial += 1u;
  }
  bvh::UpdatePerfRun(data, 5003u, 0u, base + std::chrono::milliseconds(48), off);
  const std::string text = ReadDumpText(WrittenPath(PerfResult()));
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    bvh::g_pool.episode_serial = saved_serial;
    bvh::g_pool.episode_start_frame = saved_start;
  }
  CHECK(text.find("\"stable\": false") != std::string::npos, "a switch changed during the run: stable false");
  bvh::ResetCacheDiagCounters();
}

static void TestCacheBlasWhileOff(Device* dev) {

  bvh::ResetCacheDiagCounters();
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(dev);
  bvh::g_cache_diag.enabled.store(false);
  data->live.blas_builds += 5u;  // BLAS builds while the diagnostics are off
  data->live.blas_build_ms_sum += 3.f;
  bvh::UpdateCacheDiag(dev);  // off: the counters are refreshed, nothing is attributed
  bvh::g_cache_diag.enabled.store(true);
  bvh::UpdateCacheDiag(dev);
  {
    std::lock_guard<std::mutex> lock(bvh::g_cache_diag.mutex);
    CHECK(bvh::g_cache_diag.data.work.blas_builds == 0u && bvh::g_cache_diag.data.work.blas_ms == 0.f,
          "BLAS built while off is not attributed to the first enabled present (%llu builds, %.2f ms)",
          (unsigned long long)bvh::g_cache_diag.data.work.blas_builds, (double)bvh::g_cache_diag.data.work.blas_ms);
  }
  bvh::ResetCacheDiagCounters();
}

static void TestCacheLines() {
  bvh::ResetCacheDiagCounters();
  std::vector<std::string> lines = bvh::CacheDiagLines(nullptr);
  CHECK(lines.size() == 12u && lines[0].rfind(std::string("Cache diagnostics (") + bvh::kWorldBuildId + "): ", 0) == 0 && lines[0].find(": 0 presents, tiers not computed yet") != std::string::npos,
        "header line (%zu lines)", lines.size());
  CHECK(HasLine(lines, "TLAS changes: rebuilds 0 (empty 0), added 0, removed 0, moved 0, nearest changed -, nearest add/remove -; by tier of nearest add/remove: T0 0, T1 0, T2 0, none 0"),
        "TLAS line");
  CHECK(HasLine(lines, "Re-sightings: retired 0 (retainable 0, 0 tris), new meshes 0, re-sighted 0 (0.0 %), retainable 0; near misses: count mismatch 0 (0.0 %), drift 0; tracked 0 (dropped 0, expired 0)"),
        "re-sighting line");
  CHECK(HasLine(lines, "Re-sighting gap (presents since retirement): <120 0, 120-600 0, 600-1800 0, 1800-3600 0, >=3600 0; within hold (<1800) 0 (0.0 % of new meshes); triangles not rebuilt (counts equal) 0"),
        "re-sighting gap line");
  CHECK(HasLine(lines, "Time since reset: TLAS rebuilds 0, avg 0.00 ms, max 0.00 ms; mesh capture CPU 0.00 ms over 0 meshes (decode + apply; GPU read-back wait not CPU), queue-to-applied wait avg 0.0, max 0 presents"),
        "time since reset line");
  CHECK(HasLine(lines, "Episode: none yet (starts at a map change, a pool reset or the first admitted instance)"), "episode line before any episode");
  CHECK(HasLine(lines, "Lighting gap: no data now; dark 0 of 0 presents, dark runs 0, longest dark 0, longest lit 0 presents"), "lighting line");
  CHECK(HasLine(lines, "Last file: none written in this session"), "last file line");
  {
    std::lock_guard<std::mutex> lock(bvh::g_cache_diag.mutex);
    bvh::g_cache_diag.data.tiers = TierGolden();
    bvh::g_cache_diag.data.tiers_valid = true;
    bvh::g_cache_diag.data.tlas.rebuilds = 3u;
    bvh::g_cache_diag.data.tlas.empty = 1u;
    bvh::g_cache_diag.data.tlas.added = 2u;
    bvh::g_cache_diag.data.tlas.removed = 1u;
    bvh::g_cache_diag.data.tlas.nearest_add_remove_m = 9.f;
    bvh::g_cache_diag.data.tlas.by_tier[0] = 1u;
    bvh::g_cache_diag.data.tlas.by_tier[2] = 2u;
    bvh::g_cache_diag.data.lighting.presents = 7u;
    bvh::g_cache_diag.data.lighting.dark_presents = 3u;
    bvh::g_cache_diag.data.lighting.dark_runs = 2u;
    bvh::g_cache_diag.data.lighting.longest_dark = 2u;
    bvh::g_cache_diag.data.lighting.longest_lit = 3u;
    bvh::g_cache_diag.data.lighting.dark = true;
    bvh::g_cache_diag.data.lighting.known = true;
  }
  lines = bvh::CacheDiagLines(nullptr);
  CHECK(HasLine(lines, "T0 <128 m: instances 2 (150 tris), meshes 2 (150 tris, 0.00 MB), resident 1 meshes (9 tris, 0.00 MB)"), "T0 row");
  CHECK(HasLine(lines, "Self-check: PASS (tier sums = pool totals: 5 instances, 3 meshes, 157 tris, 144 bytes)"), "self-check PASS line");
  CHECK(HasLine(lines, "Instance distance (m): <16 0, 16-32 0, 32-64 1, 64-96 0, 96-128 1, 128-192 0, 192-256 2, 256-384 1, 384-512 0, >=512 0"),
        "histogram line");
  CHECK(HasLine(lines, "TLAS changes: rebuilds 3 (empty 1), added 2, removed 1, moved 0, nearest changed -, nearest add/remove 9.0 m; by tier of nearest add/remove: T0 1, T1 0, T2 2, none 0"),
        "TLAS line with changes");
  CHECK(HasLine(lines, "Lighting gap: DARK now; dark 3 of 7 presents, dark runs 2, longest dark 2, longest lit 3 presents"), "lighting line with runs");
  {
    std::lock_guard<std::mutex> lock(bvh::g_cache_diag.mutex);
    bvh::g_cache_diag.data.tiers.self_check = false;
    bvh::g_cache_diag.data.tiers.sum_instances = 4u;
  }
  lines = bvh::CacheDiagLines(nullptr);
  CHECK(HasLine(lines, "Self-check: FAIL (tier sums 4 instances, 3 meshes, 157 tris, 144 bytes; pool totals 5, 3, 157, 144)"), "self-check FAIL line names both sums");
  {
    // An open episode shows the latest sample's fresh count; t80 is pending until the episode ends.
    std::lock_guard<std::mutex> lock(bvh::g_cache_diag.mutex);
    bvh::g_cache_diag.data.episode = bvh::CacheEpisode{};
    bvh::g_cache_diag.data.episode.ever = true;
    bvh::g_cache_diag.data.episode.open = true;
    bvh::g_cache_diag.data.episode.state = "open";
    bvh::g_cache_diag.data.episode.reason = "pool reset";
    bvh::g_cache_diag.data.episode.serial = 2u;
    bvh::g_cache_diag.data.episode.start_frame = 100u;
    bvh::g_cache_diag.data.episode.fresh_from = 100u;
    bvh::g_cache_diag.data.episode.samples.emplace_back().tlas_fresh = 7u;
  }
  lines = bvh::CacheDiagLines(nullptr);
  CHECK(HasLine(lines, "Episode: open (pool reset, serial 2, start f100, fresh from f100): open: t80 pending, fresh now 7; 1 TLAS samples"),
        "open episode line");
  bvh::ResetCacheDiagCounters();
  lines = bvh::CacheDiagLines(nullptr);
  CHECK(HasLine(lines, "Episode: none since reset"), "episode line after reset");
  bvh::g_cache_diag.data = bvh::CacheDiagData{};
}

// One fixed scenario (two meshes, instances, a retirement) with the diagnostics on or off. Returns what the TLAS,
// the mesh descriptors and the pool revision hold, plus the BLAS counters.
struct CacheRun {
  uint32_t tlas_skips = 0u;
  std::vector<bvh::WorldInstanceGPU> tlas_instances;
  std::vector<bvh::BVHNodeGPU> tlas_nodes;
  std::vector<bvh::WorldMeshGPU> mesh_descriptors;
  uint64_t revision_delta = 0u;
  uint32_t tlas_rebuilds = 0u;
  uint32_t resident = 0u;
  uint64_t meshes_uploaded = 0u;
  uint64_t triangles_uploaded = 0u;
  uint64_t blas_builds = 0u;
  uint64_t blas_built_triangles = 0u;
  uint64_t resights = 0u;
};

static CacheRun RunCacheScenario(bool enabled, bool resight = false) {
  bvh::g_cache_diag.enabled.store(enabled);
  bvh::ResetWorldPool();
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    ResetResight(&bvh::g_pool.resight);
    bvh::g_pool.resight.enabled.store(enabled);
  }
  g_obs_frame = 100000u;
  Device dev; CmdList cl; cl.dev = &dev; Queue queue; queue.dev = &dev; queue.cl = &cl;
  auto& frame = falcom_world::g_state.frame;
  frame.store(5000u);
  SetCamera(10.f, 5.f, 10.f);
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(&dev);
  uint64_t revision_start = 0u;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    revision_start = bvh::g_pool.revision;
  }
  const auto present = [&]() {
    frame.fetch_add(1u);
    bvh::UpdateLiveBvh(&dev, &queue);
    bvh::UpdateCacheDiag(&dev);
  };
  AddMesh(0x51, Grid(40, 40, 10.f, 1.f));
  AddMesh(0x52, BoxMesh(1.f));
  for (int i = 0; i < 6; ++i) AddInstance(0x51, -20.f + 7.f * static_cast<float>(i), 0.f, 30.f, 1.f, 0.2f * static_cast<float>(i));
  for (int i = 0; i < 4; ++i) AddInstance(0x52, -10.f + 6.f * static_cast<float>(i), 2.f, -15.f);
  for (int i = 0; i < 30; ++i) present();
  AddInstance(0x52, 25.f, 2.f, 25.f);
  for (int i = 0; i < 20; ++i) present();
  RetireMesh(0x52);
  for (int i = 0; i < 20; ++i) present();
  if (resight) {
    AddMesh(0x53, BoxMesh(1.f));  // same content as 0x52: a re-sighting
    for (int i = 0; i < 30; ++i) present();
  }
  CacheRun run;
  run.tlas_instances = data->tlas_instances_cpu;
  run.tlas_nodes = data->tlas_nodes_cpu;
  run.mesh_descriptors = data->mesh_descriptors;
  run.tlas_rebuilds = data->live.tlas_rebuilds;
  run.tlas_skips = data->live.tlas_skips;
  run.resident = data->live.resident_meshes;
  run.meshes_uploaded = data->live.meshes_uploaded;
  run.triangles_uploaded = data->live.triangles_uploaded;
  run.blas_builds = data->live.blas_builds;
  run.blas_built_triangles = data->live.blas_built_triangles;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    run.resights = bvh::g_pool.resight.counters.resights;
  }
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    run.revision_delta = bvh::g_pool.revision - revision_start;
  }
  bvh::DestroyBvhDeviceData(&dev);
  bvh::ResetWorldPool();
  return run;
}

static uint64_t CacheRunFingerprint(const CacheRun& run) {
  uint64_t h = 14695981039346656037ull;
  HashBytes(&h, run.tlas_instances.data(), run.tlas_instances.size() * sizeof(bvh::WorldInstanceGPU));
  HashBytes(&h, run.tlas_nodes.data(), run.tlas_nodes.size() * sizeof(bvh::BVHNodeGPU));
  HashBytes(&h, run.mesh_descriptors.data(), run.mesh_descriptors.size() * sizeof(bvh::WorldMeshGPU));
  const uint32_t rebuilds_and_skips = run.tlas_rebuilds + run.tlas_skips;  // a skip is a rebuild that was not needed
  HashBytes(&h, &rebuilds_and_skips, sizeof rebuilds_and_skips);
  HashBytes(&h, &run.resident, sizeof run.resident);
  HashBytes(&h, &run.meshes_uploaded, sizeof run.meshes_uploaded);
  HashBytes(&h, &run.revision_delta, sizeof run.revision_delta);
  HashBytes(&h, &run.blas_builds, sizeof run.blas_builds);
  return h;
}

static void CheckPerfLeg(const char* label) {
  CHECK(bvh::g_perf.run.label == label && bvh::g_perf.run.presents > 0u,
        "perf run %s counted presents in its leg (label %s, presents %llu; %s)", label, bvh::g_perf.run.label.c_str(),
        (unsigned long long)bvh::g_perf.run.presents, PerfResult().c_str());
}

static void TestCacheOnOff() {
  bvh::RequestPerfRun("pin_on_collect");
  const CacheRun on = RunCacheScenario(true);
  CHECK(CacheRunFingerprint(on) == 0x55593b70e070e7feull, "cache fingerprint ON equals HEAD pin (%016llx)", (unsigned long long)CacheRunFingerprint(on));
  CheckPerfLeg("pin_on_collect");
  bvh::RequestPerfRun("pin_off_collect");
  const CacheRun off = RunCacheScenario(false);
  bvh::g_cache_diag.enabled.store(true);
  CHECK(CacheRunFingerprint(off) == 0x55593b70e070e7feull, "cache fingerprint OFF equals HEAD pin (%016llx)", (unsigned long long)CacheRunFingerprint(off));
  CheckPerfLeg("pin_off_collect");
  bvh::g_live_bvh.skip_noop.store(true);
  bvh::RequestPerfRun("pin_on_skip");
  const CacheRun on_skip = RunCacheScenario(true);
  CHECK(CacheRunFingerprint(on_skip) == 0x55593b70e070e7feull, "cache fingerprint diag ON, skip ON equals pin (%016llx)", (unsigned long long)CacheRunFingerprint(on_skip));
  CheckPerfLeg("pin_on_skip");
  bvh::RequestPerfRun("pin_off_skip");
  const CacheRun off_skip = RunCacheScenario(false);
  bvh::g_live_bvh.skip_noop.store(false);
  bvh::g_cache_diag.enabled.store(true);
  CHECK(CacheRunFingerprint(off_skip) == 0x55593b70e070e7feull, "cache fingerprint diag OFF, skip ON equals pin (%016llx)", (unsigned long long)CacheRunFingerprint(off_skip));
  CheckPerfLeg("pin_off_skip");
  bvh::g_perf.start_requested.store(false);
  bvh::g_perf.active.store(false);
  CHECK(on.tlas_skips == 0u, "switch-OFF scenario has no skip (%u)", on.tlas_skips);
  const CacheRun on_resight = RunCacheScenario(true, true);
  const CacheRun off_resight = RunCacheScenario(false, true);
  bvh::g_cache_diag.enabled.store(true);
  CHECK(on_resight.resights >= 1u && off_resight.resights == 0u, "resight scenario: ON re-sights (%llu), OFF does not (%llu)",
        (unsigned long long)on_resight.resights, (unsigned long long)off_resight.resights);
  CHECK(SameBytes(on_resight.tlas_instances, off_resight.tlas_instances) && SameBytes(on_resight.tlas_nodes, off_resight.tlas_nodes)
            && SameBytes(on_resight.mesh_descriptors, off_resight.mesh_descriptors) && on_resight.revision_delta == off_resight.revision_delta
            && on_resight.blas_builds == off_resight.blas_builds,
        "resight scenario: TLAS, descriptors, revision and BLAS builds identical with diagnostics on/off");
  CHECK(on.tlas_instances.size() > 0u, "scenario builds a TLAS (%zu instances)", on.tlas_instances.size());
  CHECK(SameBytes(on.tlas_instances, off.tlas_instances), "TLAS instances byte-identical with diagnostics on/off");
  CHECK(SameBytes(on.tlas_nodes, off.tlas_nodes), "TLAS nodes byte-identical with diagnostics on/off");
  CHECK(SameBytes(on.mesh_descriptors, off.mesh_descriptors), "mesh descriptors byte-identical with diagnostics on/off");
  CHECK(on.revision_delta == off.revision_delta, "pool revision delta equal (%llu, %llu)", (unsigned long long)on.revision_delta, (unsigned long long)off.revision_delta);
  CHECK(on.tlas_rebuilds == off.tlas_rebuilds && on.resident == off.resident && on.meshes_uploaded == off.meshes_uploaded,
        "rebuilds, resident and uploads equal (%u/%u, %u/%u)", on.tlas_rebuilds, off.tlas_rebuilds, on.resident, off.resident);
  CHECK(on.blas_builds == off.blas_builds && on.blas_builds >= on.meshes_uploaded, "BLAS counters always on (%llu, %llu)", (unsigned long long)on.blas_builds,
        (unsigned long long)off.blas_builds);
  CHECK(on.blas_built_triangles == on.triangles_uploaded, "BLAS triangles equal triangles uploaded (%llu, %llu)",
        (unsigned long long)on.blas_built_triangles, (unsigned long long)on.triangles_uploaded);
  std::printf("cache diag on/off: TLAS %zu instances, %u rebuilds, BLAS %llu builds / %llu triangles\n", on.tlas_instances.size(),
              on.tlas_rebuilds, (unsigned long long)on.blas_builds, (unsigned long long)on.blas_built_triangles);
}

static void TestCacheEnteredIds() {
  const std::vector<bvh::LiveTlasInstance> before = {TlasAt(1u, 1.f), TlasAt(2u, 2.f), TlasAt(3u, 3.f, true)};
  const std::vector<bvh::LiveTlasInstance> after = {TlasAt(5u, 5.f), TlasAt(2u, 2.f), TlasAt(0u, 0.f), TlasAt(4u, 4.f, true)};
  const std::vector<uint64_t> entered = bvh::CacheEnteredIds(before, after);
  CHECK(entered == std::vector<uint64_t>({4u, 5u}), "entered ids: wind included, id 0 skipped, ascending (%zu)", entered.size());
  CHECK(bvh::CacheEnteredIds(after, before) == std::vector<uint64_t>({1u, 3u}), "left ids are not entered");
}

static void TestCacheLatencyBuckets() {
  const float frames[9] = {0.f, 2.f, 3.f, 8.f, 9.f, 30.f, 31.f, 120.f, 121.f};
  const size_t expected[9] = {0u, 0u, 1u, 1u, 2u, 2u, 3u, 3u, 4u};
  for (size_t i = 0; i < 9; ++i) {
    const size_t bucket = bvh::CacheBucket(frames[i], bvh::kCacheLatencyEdges, std::size(bvh::kCacheLatencyEdges));
    CHECK(bucket == expected[i], "latency %.0f presents in bucket %zu (expected %zu)", (double)frames[i], bucket, expected[i]);
  }
}

// Synthetic TLAS: instance 2 is dynamic (the suffix), so a pose change there is not a rebuild.
static void TestLiveSnapshotMatches() {
  bvh::BvhDeviceData data;
  std::vector<bvh::WorldInstanceGPU> gpu(3);
  std::vector<bvh::LiveTlasInstance> info(3);
  for (size_t i = 0; i < 3; ++i) {
    gpu[i].header[0] = static_cast<float>(i + 1);
    gpu[i].world[12] = static_cast<float>(i);
    info[i].id = 10u + i;
  }
  info[2].dynamic = true;
  data.tlas_built = true;
  data.tlas_dynamic_first = 2u;
  data.tlas_instances_cpu = gpu;
  data.tlas_info = info;
  const auto matches = [&data](const std::vector<bvh::WorldInstanceGPU>& g, const std::vector<bvh::LiveTlasInstance>& i, bool dynamic_poses) {
    return bvh::LiveSnapshotMatchesTlas(data, g, i, dynamic_poses);
  };
  CHECK(matches(gpu, info, true) && matches(gpu, info, false), "identical snapshot matches");
  std::vector<bvh::WorldInstanceGPU> moved = gpu;
  moved[0].world[3] += 1.f;
  CHECK(!matches(moved, info, false), "static matrix change is not a no-op");
  std::vector<bvh::WorldInstanceGPU> visible = gpu;
  visible[1].visibility[0] += 1.f;
  CHECK(!matches(visible, info, false), "static visibility change is not a no-op");
  std::vector<bvh::WorldInstanceGPU> slot = gpu;
  slot[0].header[0] += 1.f;
  CHECK(!matches(slot, info, false), "static slot change is not a no-op");
  std::vector<bvh::WorldInstanceGPU> pose = gpu;
  pose[2].world[3] += 1.f;
  CHECK(!matches(pose, info, true) && matches(pose, info, false), "dynamic pose change: full differs, static equal");
  std::vector<bvh::LiveTlasInstance> renamed = info;
  renamed[0].id = 99u;
  CHECK(!matches(gpu, renamed, false), "same descriptor with another id is not a no-op");
  std::vector<bvh::LiveTlasInstance> winded = info;
  winded[2].wind = true;
  CHECK(!matches(gpu, winded, false), "dynamic instance with another wind flag is not a no-op");
  std::vector<bvh::WorldInstanceGPU> seen = gpu;
  seen[2].visibility[0] += 1.f;
  CHECK(!matches(seen, info, false), "dynamic instance with another visibility is not a no-op");
}

// Live: a no-op rebuild (admit bump, no content change) is counted; an instance admitted later enters at its own latency bucket.
static void TestCacheRebuildDiag() {
  bvh::g_cache_diag.enabled.store(true);
  bvh::g_live_bvh.skip_noop.store(false);  // Step 0 rebuild path: a no-op is rebuilt and counted as one
  bvh::ResetWorldPool();
  bvh::ResetCacheDiagCounters();
  g_obs_frame = 200000u;
  Device dev; CmdList cl; cl.dev = &dev; Queue queue; queue.dev = &dev; queue.cl = &cl;
  auto& frame = falcom_world::g_state.frame;
  frame.store(7000u);
  SetCamera(10.f, 5.f, 10.f);
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(&dev);
  const auto present = [&]() {
    frame.fetch_add(1u);
    bvh::UpdateLiveBvh(&dev, &queue);
    bvh::UpdateCacheDiag(&dev);
  };
  AddMesh(0x71, BoxMesh(1.f));
  for (int i = 0; i < 3; ++i) AddInstance(0x71, 2.f * static_cast<float>(i), 0.f, 20.f);
  for (int i = 0; i < 5 && data->live.tlas_rebuilds == 0u; ++i) present();
  CHECK(data->live.tlas_rebuilds == 1u, "first TLAS (%u rebuilds)", data->live.tlas_rebuilds);
  {
    std::lock_guard lock(bvh::g_cache_diag.mutex);
    const bvh::CacheTlasStats& t = bvh::g_cache_diag.data.tlas;
    float sum = 0.f;
    bool finite = true;
    for (size_t p = 0; p < bvh::kCachePhaseCount; ++p) {
      finite = finite && std::isfinite(t.phase_last[p]);
      sum += t.phase_last[p];
    }
    CHECK(finite && sum <= data->live.tlas_ms_last + 0.3f, "rebuild phases finite, sum %.3f ms within TLAS %.3f ms", (double)sum, (double)data->live.tlas_ms_last);
    CHECK(t.by_reason[0] == 1u, "first build counted under its reason");
  }

  const uint32_t rebuilds_before = data->live.tlas_rebuilds;
  uint64_t noop_before = 0u;
  {
    std::lock_guard lock(bvh::g_cache_diag.mutex);
    noop_before = bvh::g_cache_diag.data.tlas.noop_full;
  }
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    bvh::BumpPoolRevision(bvh::PoolRevisionSource::Admit);
  }
  for (int i = 0; i < 12 && data->live.tlas_rebuilds == rebuilds_before; ++i) present();
  CHECK(data->live.tlas_rebuilds == rebuilds_before + 1u, "admit bump rebuilds the TLAS (%u)", data->live.tlas_rebuilds - rebuilds_before);
  {
    std::lock_guard lock(bvh::g_cache_diag.mutex);
    const bvh::CacheTlasStats& t = bvh::g_cache_diag.data.tlas;
    CHECK(t.noop_full == noop_before + 1u && t.noop_static == t.noop_full, "no-op rebuild counted (full %llu, static %llu)",
          (unsigned long long)t.noop_full, (unsigned long long)t.noop_static);
  }

  size_t latency_bucket = 0u;
  uint64_t entered_before = 0u;
  {
    std::lock_guard lock(bvh::g_cache_diag.mutex);
    entered_before = bvh::g_cache_diag.data.tlas.entered;
  }
  AddInstance(0x71, 40.f, 0.f, 20.f);
  uint64_t new_id = 0u;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    new_id = bvh::g_pool.instances.back().id;
  }
  for (int i = 0; i < 20; ++i) {
    present();
    std::lock_guard lock(bvh::g_pool.mutex);
    const bvh::WorldInstance& instance = bvh::g_pool.instances.back();
    if (instance.tlas_entry_frame == 0u) continue;
    latency_bucket = bvh::CacheBucket(static_cast<float>(instance.tlas_entry_frame - instance.admit_frame), bvh::kCacheLatencyEdges, std::size(bvh::kCacheLatencyEdges));
    break;
  }
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    CHECK(bvh::g_pool.instances.back().id == new_id && bvh::g_pool.instances.back().tlas_entry_frame != 0u, "new instance entered the TLAS");
  }
  {
    std::lock_guard lock(bvh::g_cache_diag.mutex);
    const bvh::CacheTlasStats& t = bvh::g_cache_diag.data.tlas;
    CHECK(t.entered == entered_before + 1u && t.admit_latency[latency_bucket] >= 1u, "entry counted in its latency bucket (%zu)", latency_bucket);
  }
  bvh::DestroyBvhDeviceData(&dev);
  bvh::ResetWorldPool();
  bvh::ResetCacheDiagCounters();
  bvh::g_live_bvh.skip_noop.store(false);
}

// One mesh and a TLAS over it, for the skip tests (own device; the pool is reset).
struct SkipRig {
  Device dev;
  CmdList cl;
  Queue queue;
  SkipRig() {
    cl.dev = &dev;
    queue.dev = &dev;
    queue.cl = &cl;
    bvh::g_cache_diag.enabled.store(true);
    bvh::g_live_bvh.skip_noop.store(true);
    bvh::ResetWorldPool();
    bvh::ResetCacheDiagCounters();
    g_obs_frame += 1000u;
    falcom_world::g_state.frame.store(9000u);
    SetCamera(10.f, 5.f, 10.f);
    AddMesh(0x81, BoxMesh(1.f));
    AddMesh(0x82, BoxMesh(2.f));
  }
  ~SkipRig() {
    bvh::g_live_bvh.skip_noop.store(false);
    bvh::DestroyBvhDeviceData(&dev);
    bvh::ResetWorldPool();
    bvh::ResetCacheDiagCounters();
  }
  bvh::BvhDeviceData* data() { return bvh::GetBvhDeviceData(&dev); }
  void present() {
    falcom_world::g_state.frame.fetch_add(1u);
    bvh::UpdateLiveBvh(&dev, &queue);
    bvh::UpdateCacheDiag(&dev);
  }
};

static void BumpAdmit() {
  std::lock_guard lock(bvh::g_pool.mutex);
  bvh::BumpPoolRevision(bvh::PoolRevisionSource::Admit);
}

// A bump with no content change is skipped: no build, no buffer, the TLAS bytes stay, the pending sources are taken.
static void TestSkipNoop() {
  SkipRig rig;
  AddInstance(0x81, 0.f, 0.f, 20.f);
  AddInstance(0x81, 3.f, 0.f, 20.f);
  AddInstance(0x82, -3.f, 0.f, 20.f);
  for (int i = 0; i < 3 && rig.data()->live.tlas_rebuilds == 0u; ++i) rig.present();
  CHECK(rig.data()->live.tlas_rebuilds == 1u && rig.data()->tlas_built, "first TLAS before the skip test");
  for (int i = 0; i < 10; ++i) rig.present();
  const uint32_t rebuilds = rig.data()->live.tlas_rebuilds;
  const std::vector<bvh::WorldInstanceGPU> instances = rig.data()->tlas_instances_cpu;
  const std::vector<bvh::BVHNodeGPU> nodes = rig.data()->tlas_nodes_cpu;
  const std::vector<bvh::BVHLeafGPU> leaves = rig.data()->tlas_leaves_cpu;
  const size_t resources = rig.dev.res.size();
  BumpAdmit();
  for (int i = 0; i < 12 && rig.data()->live.tlas_skips == 0u; ++i) rig.present();
  CHECK(rig.data()->live.tlas_skips == 1u && rig.data()->live.tlas_rebuilds == rebuilds, "admit bump with no content change is skipped (skips %u, rebuilds %u -> %u)",
        rig.data()->live.tlas_skips, rebuilds, rig.data()->live.tlas_rebuilds);
  CHECK(SameBytes(instances, rig.data()->tlas_instances_cpu) && SameBytes(nodes, rig.data()->tlas_nodes_cpu)
            && SameBytes(leaves, rig.data()->tlas_leaves_cpu),
        "TLAS instances, nodes and leaves unchanged by the skip");
  CHECK(rig.dev.res.size() == resources, "skip creates and destroys no buffer (%zu -> %zu)", resources, rig.dev.res.size());
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    CHECK(bvh::g_pool.revision_sources_pending == 0u && rig.data()->tlas_pool_revision == bvh::g_pool.revision,
          "sources taken and revision reflected by the skip");
  }
  for (int i = 0; i < 6; ++i) rig.present();
  CHECK(rig.data()->live.tlas_skips == 1u && rig.data()->live.tlas_rebuilds == rebuilds, "no further snapshot before the next gate");
}

// Real changes (add, retire) are never skipped; the result equals a run with the switch off.
static void SkipSequence(bool skip, std::vector<bvh::WorldInstanceGPU>* out, uint32_t* rebuilds, uint32_t* skips) {
  SkipRig rig;
  bvh::g_live_bvh.skip_noop.store(skip);
  AddInstance(0x81, 0.f, 0.f, 20.f);
  AddInstance(0x82, -3.f, 0.f, 20.f);
  for (int i = 0; i < 10; ++i) rig.present();
  AddInstance(0x81, 6.f, 0.f, 20.f);
  for (int i = 0; i < 10; ++i) rig.present();
  RetireMesh(0x82);
  for (int i = 0; i < 10; ++i) rig.present();
  *out = rig.data()->tlas_instances_cpu;
  *rebuilds = rig.data()->live.tlas_rebuilds;
  *skips = rig.data()->live.tlas_skips;
  bvh::g_live_bvh.skip_noop.store(true);
}

static void TestSkipDifferent() {
  std::vector<bvh::WorldInstanceGPU> on;
  std::vector<bvh::WorldInstanceGPU> off;
  uint32_t on_rebuilds = 0u, on_skips = 0u, off_rebuilds = 0u, off_skips = 0u;
  SkipSequence(true, &on, &on_rebuilds, &on_skips);
  SkipSequence(false, &off, &off_rebuilds, &off_skips);
  CHECK(on_rebuilds == off_rebuilds && on_rebuilds >= 3u, "add and retire rebuild with skip on (%u, off %u)", on_rebuilds, off_rebuilds);
  CHECK(on_skips == 0u && off_skips == 0u, "no skip for real changes (on %u, off %u)", on_skips, off_skips);
  CHECK(SameBytes(on, off), "TLAS with skip on equals the switch-off run");
}

// A moved dynamic instance with a bump: the snapshot is skipped, the dynamic revision stays behind so the refit still runs.
static void TestSkipDynamicPose() {
  SkipRig rig;
  AddInstance(0x81, 5.f, 0.f, 5.f);
  AddInstance(0x82, -5.f, 0.f, 5.f);
  for (int i = 0; i < 12; ++i) rig.present();
  MoveDynamicInstance(0x81, 5.f, 5.f, falcom_world::g_state.frame.load());
  for (int i = 0; i < 10; ++i) rig.present();
  const uint32_t skips = rig.data()->live.tlas_skips;
  const uint32_t rebuilds = rig.data()->live.tlas_rebuilds;
  const uint32_t refits = rig.data()->live.tlas_refits;
  MoveDynamicInstance(0x81, 5.4f, 5.f, falcom_world::g_state.frame.load());
  BumpAdmit();
  for (int i = 0; i < 12 && rig.data()->live.tlas_skips == skips; ++i) rig.present();
  CHECK(rig.data()->live.tlas_skips == skips + 1u && rig.data()->live.tlas_rebuilds == rebuilds, "dynamic pose with a bump: skipped");
  uint64_t pool_dynamic_revision = 0u;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    pool_dynamic_revision = bvh::g_pool.dynamic_revision;
  }
  CHECK(rig.data()->live.tlas_refits > refits && rig.data()->tlas_dynamic_revision == pool_dynamic_revision,
        "refit in the skipped present: the pose follows this frame (%u refits)", rig.data()->live.tlas_refits - refits);
  CHECK(CompareTraces(rig.dev, *rig.data(), "skip dynamic", 200, 3) == 0, "trace agrees after a skipped snapshot and a refit");
}

// 120 refits force a full rebuild for tree quality, a skip never hides it.
static void TestSkipQuality() {
  SkipRig rig;
  AddInstance(0x81, 5.f, 0.f, 5.f);
  AddInstance(0x82, -5.f, 0.f, 5.f);
  for (int i = 0; i < 12; ++i) rig.present();
  MoveDynamicInstance(0x81, 5.4f, 5.f, falcom_world::g_state.frame.load());
  for (int i = 0; i < 10; ++i) rig.present();
  MoveDynamicInstance(0x81, 5.f, 5.f, falcom_world::g_state.frame.load());
  rig.data()->tlas_refits_since_rebuild = bvh::kLiveTlasRefitsPerRebuild;
  const uint32_t skips = rig.data()->live.tlas_skips;
  const uint32_t rebuilds = rig.data()->live.tlas_rebuilds;
  for (int i = 0; i < 12 && rig.data()->live.tlas_rebuilds == rebuilds; ++i) rig.present();
  CHECK(rig.data()->live.tlas_rebuilds == rebuilds + 1u && std::string(rig.data()->live.tlas_reason) == "refit quality" && rig.data()->live.tlas_skips == skips,
        "refit quality rebuilds (reason %s, skips %u)", rig.data()->live.tlas_reason, rig.data()->live.tlas_skips);
}

// A skip at F moves no gate: a change right after it rebuilds at F + kLiveTlasInterval, as HEAD does.
static void TestSkipCadence() {
  SkipRig rig;
  AddInstance(0x81, 0.f, 0.f, 20.f);
  for (int i = 0; i < 12 && rig.data()->live.tlas_rebuilds == 0u; ++i) rig.present();
  for (int i = 0; i < 9; ++i) rig.present();
  BumpAdmit();
  for (int i = 0; i < 12 && rig.data()->live.tlas_skips == 0u; ++i) rig.present();
  CHECK(rig.data()->live.tlas_skips == 1u, "bump skipped");
  const uint32_t check = rig.data()->tlas_check_frame;
  const uint32_t rebuilds = rig.data()->live.tlas_rebuilds;
  AddInstance(0x82, 3.f, 0.f, 20.f);
  for (int i = 0; i < 12 && rig.data()->live.tlas_rebuilds == rebuilds; ++i) rig.present();
  CHECK(rig.data()->live.tlas_rebuilds == rebuilds + 1u && rig.data()->live.tlas_frame == check + bvh::kLiveTlasInterval,
        "change after a skip rebuilds at +%u (check f%u)", rig.data()->live.tlas_frame - check, check);
}

// Slack TLAS (session switch): a skip rig with the switch on; the static block gets holes at the first full build.
struct SlackRig : SkipRig {
  SlackRig() { bvh::g_live_bvh.slack.store(true); }
  ~SlackRig() { bvh::g_live_bvh.slack.store(false); }
};

static uint64_t SlackKey(uint32_t mesh) { return 0x300u + mesh; }

// 36 meshes of 30 instances (1080 static instances): above kLiveSlackMinStatic, so the first build has 256 holes.
static void SlackFill(SlackRig* rig) {
  for (uint32_t m = 0; m < 36u; ++m) {
    AddMesh(SlackKey(m), BoxMesh(0.5f + 0.001f * static_cast<float>(m)));  // 1 m boxes, 4 m apart: no overlaps (ties would differ from brute force)
    for (uint32_t i = 0; i < 30u; ++i) {
      const float x = -60.f + 4.f * static_cast<float>(i % 15u);
      const float z = 20.f + 4.f * static_cast<float>(m) + 60.f * static_cast<float>(i / 15u);
      AddInstance(SlackKey(m), x, 0.f, z);
    }
  }
  for (int i = 0; i < 3 && rig->data()->live.tlas_rebuilds == 0u; ++i) rig->present();
}

// Moves every instance admitted through `key` by dx in x: a static change (no dynamic flag), with a pool revision.
static void MoveStaticInstances(uint64_t key, float dx) {
  std::lock_guard lock(bvh::g_pool.mutex);
  for (bvh::WorldInstance& inst : bvh::g_pool.instances) {
    if (inst.mesh_key != key) continue;
    float mn[3], mx[3];
    bvh::g_pool.admitted_keys.erase(bvh::PoolAdmittedKey(inst.mesh_id, falcom_world::MatrixHash(inst.matrix, 12)));
    inst.matrix[3] += dx;
    falcom_world::ComputeMatrixInverse(inst.matrix, inst.inverse_world);
    bvh::ComputePoolInstanceBounds(bvh::g_pool.meshes[inst.mesh_id], inst.matrix, mn, mx);
    std::memcpy(inst.bounds_min, mn, sizeof(mn));
    std::memcpy(inst.bounds_max, mx, sizeof(mx));
    bvh::g_pool.admitted_keys.insert(bvh::PoolAdmittedKey(inst.mesh_id, falcom_world::MatrixHash(inst.matrix, 12)));
  }
  bvh::g_pool.revision += 1u;
}

static void TestSlackLayout() {
  SlackRig rig;
  SlackFill(&rig);
  const bvh::BvhDeviceData* data = rig.data();
  CHECK(data->tlas_slack && data->tlas_free_slots.size() == 256u && data->tlas_instances_cpu.size() == 1080u + 256u,
        "slack layout: %zu free holes, %zu slots", data->tlas_free_slots.size(), data->tlas_instances_cpu.size());
  CHECK(data->active_count == 1080u && data->tlas_dynamic_first == 1080u + 256u && data->tlas_slot_of_id.size() == 1080u,
        "live count %u, dynamic first %u, ids mapped %zu", data->active_count, data->tlas_dynamic_first, data->tlas_slot_of_id.size());
  bool holes_zero = true;
  for (uint32_t s = 1080u; s < 1080u + 256u; ++s) holes_zero = holes_zero && data->tlas_info[s].id == 0u && data->tlas_leaves_cpu.size() > s;
  CHECK(holes_zero, "holes have no id");
  bvh::g_live_bvh.check_requested.store(true);
  rig.present();
  CHECK(data->live.gpu_checked && data->live.gpu_ok, "GPU check with holes: %s", data->live.gpu_result.c_str());
}

// Random static changes (add, retire, move) applied in place; each result traces like brute force, the GPU contents match.
static void TestSlackApply() {
  SlackRig rig;
  SlackFill(&rig);
  const bvh::BvhDeviceData* data = rig.data();
  Device& dev = rig.dev;
  uint32_t seed = 12345u;
  for (uint32_t step = 0; step < 24u; ++step) {
    seed = seed * 1664525u + 1013904223u;
    const uint32_t action = (seed >> 8) % 3u;
    const uint32_t mesh = (seed >> 16) % 36u;
    if (action == 0u) {
      const uint64_t key = 0x400u + step;
      AddMesh(key, BoxMesh(0.6f + 0.001f * static_cast<float>(step)));
      for (uint32_t i = 0; i < 3u; ++i) AddInstance(key, 100.f + 3.f * static_cast<float>(i), 0.f, 100.f + 3.f * static_cast<float>(step));
    } else if (action == 1u) {
      RetireMesh(SlackKey(mesh));
    } else {
      MoveStaticInstances(SlackKey(mesh), 0.5f + 0.1f * static_cast<float>(step));
    }
    for (int i = 0; i < 9; ++i) rig.present();
    CHECK(CompareTraces(dev, *data, "slack step", 120, 40 + step) == 0, "slack step %u traces like brute force (action %u, applies %u)", step, action,
        data->live.slack_applies);
  }
  CHECK(data->live.slack_applies >= 5u, "in-place applies (%u, added %u, removed %u, changed %u, free %u)", data->live.slack_applies,
        data->live.slack_added, data->live.slack_removed, data->live.slack_changed, data->live.slack_free_holes);
  bvh::g_live_bvh.check_requested.store(true);
  rig.present();
  CHECK(data->live.gpu_checked && data->live.gpu_ok, "GPU check after applies: %s", data->live.gpu_result.c_str());
}

// More changes in one step than kLiveSlackMaxOps: a full rebuild, with the reason "slack exhausted"; the rebuilt TLAS has its holes again.
static void TestSlackExhaustion() {
  SlackRig rig;
  SlackFill(&rig);
  bvh::BvhDeviceData* data = rig.data();
  for (uint32_t m = 0; m < 20u; ++m) MoveStaticInstances(SlackKey(m), 0.2f);  // 600 moved instances
  const uint32_t rebuilds = data->live.tlas_rebuilds;
  for (int i = 0; i < 12 && data->live.tlas_rebuilds == rebuilds; ++i) rig.present();
  CHECK(data->live.tlas_rebuilds == rebuilds + 1u && std::string(data->live.tlas_reason) == "slack exhausted" && data->live.slack_exhausted >= 1u,
        "exhausted: rebuild with reason %s (%u)", data->live.tlas_reason, data->live.slack_exhausted);
  CHECK(data->tlas_slack && data->tlas_free_slots.size() == 256u && data->active_count == 1080u, "holes restored (%zu free, %u live)",
        data->tlas_free_slots.size(), data->active_count);
  CHECK(CompareTraces(rig.dev, *data, "exhausted", 120, 77) == 0, "trace agrees after the exhaustion rebuild");
}

// A change of the dynamic set (a promotion) is structural: a full rebuild, never an in-place update.
static void TestSlackStructural() {
  SlackRig rig;
  SlackFill(&rig);
  bvh::BvhDeviceData* data = rig.data();
  const uint32_t rebuilds = data->live.tlas_rebuilds;
  const uint32_t structural = data->live.slack_structural;
  MoveDynamicInstance(SlackKey(0u), 0.f, 0.f, falcom_world::g_state.frame.load());
  for (int i = 0; i < 12 && data->live.tlas_rebuilds == rebuilds; ++i) rig.present();
  CHECK(data->live.tlas_rebuilds == rebuilds + 1u && data->live.slack_structural == structural + 1u
            && std::string(data->live.tlas_reason) == "slack structural",
        "promotion is structural (reason %s, structural %u)", data->live.tlas_reason, data->live.slack_structural);
}

// A pool change and a region move in the same present: a full rebuild, not an in-place update.
static void TestSlackRegionMasked() {
  SlackRig rig;
  SlackFill(&rig);
  bvh::BvhDeviceData* data = rig.data();
  const uint32_t applies = data->live.slack_applies;
  const uint32_t rebuilds = data->live.tlas_rebuilds;
  AddInstance(SlackKey(1u), 0.f, 0.f, 500.f);
  SetCamera(400.f, 5.f, 10.f);  // the region leaves most of the static block: the snapshot changes too
  for (int i = 0; i < 12 && data->live.tlas_rebuilds == rebuilds; ++i) rig.present();
  CHECK(data->live.tlas_rebuilds == rebuilds + 1u && data->live.slack_applies == applies,
        "pool change and region move: rebuilt, not applied (reason %s; rebuilds %u, skips %u, applies %u, frame %u, tlas_frame %u, check %u, waiting %u, instances %u)",
        data->live.tlas_reason, data->live.tlas_rebuilds, data->live.tlas_skips, data->live.slack_applies, falcom_world::g_state.frame.load(),
        data->live.tlas_frame, data->tlas_check_frame, data->live.tlas_waiting, data->live.tlas_instances);
}

// Switch off: the next rebuild is the plain one (no holes, reason "slack switch"); the traces still agree.
static void TestSlackToggle() {
  SlackRig rig;
  SlackFill(&rig);
  bvh::BvhDeviceData* data = rig.data();
  AddInstance(SlackKey(2u), 0.f, 0.f, 300.f);
  for (int i = 0; i < 12 && data->live.slack_applies == 0u; ++i) rig.present();
  CHECK(data->live.slack_applies >= 1u, "an in-place apply before the switch");
  bvh::g_live_bvh.slack.store(false);
  const uint32_t rebuilds = data->live.tlas_rebuilds;
  for (int i = 0; i < 12 && data->live.tlas_rebuilds == rebuilds; ++i) rig.present();
  CHECK(data->live.tlas_rebuilds == rebuilds + 1u && std::string(data->live.tlas_reason) == "slack switch", "switch off rebuilds (reason %s)",
        data->live.tlas_reason);
  CHECK(!data->tlas_slack && data->tlas_free_slots.empty() && data->active_count == data->tlas_instances_cpu.size(),
        "no holes after the switch (%u live of %zu)", data->active_count, data->tlas_instances_cpu.size());
  CHECK(CompareTraces(rig.dev, *data, "switch off", 120, 88) == 0, "trace agrees after the switch");
}

// 120 frames without a full rebuild: the next change is a full rebuild for tree quality.
static void TestSlackMaintenance() {
  SlackRig rig;
  SlackFill(&rig);
  bvh::BvhDeviceData* data = rig.data();
  const uint32_t maintenance = data->live.slack_maintenance;
  for (uint32_t step = 0; step < 16u; ++step) {
    AddInstance(SlackKey(3u), 0.f, 0.f, 400.f + static_cast<float>(step));
    for (int i = 0; i < 9; ++i) rig.present();
  }
  CHECK(data->live.slack_maintenance > maintenance, "maintenance rebuild after 120 frames (%u)", data->live.slack_maintenance);
}

// Slack switch: ticking it on an unchanged scene rebuilds (with holes, once), it is not skipped; unticking rebuilds without holes.
static void TestSlackSwitchSkip() {
  {
    SlackRig rig;
    bvh::g_live_bvh.slack.store(false);
    SlackFill(&rig);
    bvh::BvhDeviceData* data = rig.data();
    uint32_t rebuilds = data->live.tlas_rebuilds;
    const uint32_t skips = data->live.tlas_skips;
    bvh::g_live_bvh.slack.store(true);
    for (int i = 0; i < 12 && data->live.tlas_rebuilds == rebuilds; ++i) rig.present();
    CHECK(data->live.tlas_rebuilds == rebuilds + 1u && std::string(data->live.tlas_reason) == "slack switch", "slack on: rebuilt, not skipped (%s)",
          data->live.tlas_reason);
    CHECK(data->tlas_slack && data->tlas_free_slots.size() == 256u, "slack on: holes built (%zu)", data->tlas_free_slots.size());
    for (int i = 0; i < 12; ++i) rig.present();
    CHECK(data->live.tlas_skips == skips, "slack on, unchanged scene: no repeated skips (%u -> %u)", skips, data->live.tlas_skips);
    rebuilds = data->live.tlas_rebuilds;
    bvh::g_live_bvh.slack.store(false);
    for (int i = 0; i < 12 && data->live.tlas_rebuilds == rebuilds; ++i) rig.present();
    CHECK(data->live.tlas_rebuilds == rebuilds + 1u && std::string(data->live.tlas_reason) == "slack switch" && !data->tlas_slack
              && data->tlas_free_slots.empty(),
          "slack off: rebuilt without holes (%s)", data->live.tlas_reason);
  }
  {
    SkipRig rig;
    AddInstance(0x81, 0.f, 0.f, 20.f);
    AddInstance(0x81, 3.f, 0.f, 20.f);
    for (int i = 0; i < 12 && rig.data()->live.tlas_rebuilds == 0u; ++i) rig.present();
    uint32_t rebuilds = rig.data()->live.tlas_rebuilds;
    bvh::g_live_bvh.slack.store(true);  // a small TLAS: no holes, the switch still rebuilds
    for (int i = 0; i < 12 && rig.data()->live.tlas_rebuilds == rebuilds; ++i) rig.present();
    CHECK(rig.data()->live.tlas_rebuilds == rebuilds + 1u && std::string(rig.data()->live.tlas_reason) == "slack switch",
          "small scene, slack on: rebuilt (%s)", rig.data()->live.tlas_reason);
    rebuilds = rig.data()->live.tlas_rebuilds;
    bvh::g_live_bvh.slack.store(false);
    for (int i = 0; i < 12 && rig.data()->live.tlas_rebuilds == rebuilds; ++i) rig.present();
    CHECK(rig.data()->live.tlas_rebuilds == rebuilds + 1u && std::string(rig.data()->live.tlas_reason) == "slack switch" && !rig.data()->tlas_slack,
          "small scene, slack off: rebuilt (%s)", rig.data()->live.tlas_reason);
  }
}

// Diagnostics: an in-place apply is counted as an apply and does not move the rebuild counters or the reasons.
static void TestSlackAppliesDiag() {
  SlackRig rig;
  SlackFill(&rig);
  bvh::BvhDeviceData* data = rig.data();
  AddMesh(0x600u, BoxMesh(0.8f));
  AddInstance(0x600u, 150.f, 0.f, 150.f);
  const auto counts = []() {
    std::lock_guard lock(bvh::g_cache_diag.mutex);
    const bvh::CacheTlasStats& t = bvh::g_cache_diag.data.tlas;
    uint64_t reasons = 0u;
    for (const uint64_t count : t.by_reason) reasons += count;
    return std::vector<uint64_t>{t.applies, t.rebuilds, bvh::g_cache_diag.data.work.tlas_rebuilds, reasons};
  };
  const std::vector<uint64_t> before = counts();
  const uint32_t live_before = data->live.slack_applies;
  for (int i = 0; i < 20 && data->live.slack_applies == live_before; ++i) rig.present();
  const std::vector<uint64_t> after = counts();
  CHECK(data->live.slack_applies > live_before, "an in-place apply happened (%u)", data->live.slack_applies);
  CHECK(after[0] - before[0] == data->live.slack_applies - live_before && after[0] - before[0] >= 1u,
        "tlas.applies moved with the applies (%llu vs %u)", (unsigned long long)(after[0] - before[0]), data->live.slack_applies - live_before);
  CHECK(after[1] == before[1] && after[2] == before[2] && after[3] == before[3],
        "applies leave rebuilds, work rebuilds and reasons unchanged (%llu, %llu, %llu)", (unsigned long long)(after[1] - before[1]),
        (unsigned long long)(after[2] - before[2]), (unsigned long long)(after[3] - before[3]));
  // Apply cost (every successful apply, zero-op applies included; the count equals slack_applies by construction in RebuildLiveTlas).
  CHECK(data->live.slack_apply_ms_sum > 0.f && data->live.slack_apply_ms_max > 0.f, "apply ms recorded after an apply (sum %.4f, max %.4f)", data->live.slack_apply_ms_sum, data->live.slack_apply_ms_max);
  CHECK(data->live.slack_apply_ms_max <= data->live.slack_apply_ms_sum, "apply ms: max %.4f <= sum %.4f", data->live.slack_apply_ms_max, data->live.slack_apply_ms_sum);
  const std::string text = ReadDumpText(bvh::WriteCacheDiagFile(data, 1u, "", bvh::CacheSwitches{}));
  const std::string slack = "\"slack\": {\"applies\": " + std::to_string(data->live.slack_applies) + ", \"apply_ms_sum\": ";
  CHECK(text.find(slack) != std::string::npos, "JSON slack block equals the live applies (%u)", data->live.slack_applies);
}

int main() {
  std::setvbuf(stdout, nullptr, _IONBF, 0);
  Stage("main start");
  if (g_quick) std::printf("QUICK: not a full verification\n");
  Device dev; CmdList cl; cl.dev = &dev; Queue queue; queue.dev = &dev; queue.cl = &cl;
  bvh::g_pool.region_size = 512.f;
  SetCamera(10.f, 5.f, 10.f);
  auto& frame = falcom_world::g_state.frame;
  frame.store(1000u);
  auto present = [&]() { frame.fetch_add(1u); bvh::UpdateLiveBvh(&dev, &queue); };
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(&dev);

  // Meshes in pool order: A 180k triangles, B 30k, C 20k (each over the budget alone), D 12, E 28.8k.
  const uint64_t kA = 0xA, kB = 0xB, kC = 0xC, kD = 0xD, kE = 0xE;
  Stage("adding meshes");
  AddMesh(kA, Grid(300, 300, 40.f, 2.f));
  AddMesh(kB, Grid(150, 100, 20.f, 1.f));
  AddMesh(kC, Grid(100, 100, 10.f, 1.f));
  AddMesh(kD, BoxMesh(1.f));
  AddMesh(kE, Grid(120, 120, 15.f, 3.f));
  Stage("adding instances");
  AddInstance(kA, 0, 0, 0);
  AddInstance(kA, 60, -3, 40, 1.f, 0.5f);
  for (int i = 0; i < 5; ++i) AddInstance(kB, -60.f + i * 25.f, 2, -50, 1.f, 0.3f * i);
  for (int i = 0; i < 5; ++i) AddInstance(kC, 30.f + i * 12.f, 4, 80, 0.5f + 0.2f * i, 0.1f * i);
  for (int i = 0; i < 20; ++i) AddInstance(kD, -40.f + (i % 5) * 9.f, 6.f + (i / 5) * 3.f, 20.f + (i % 3) * 7.f, 1.f + 0.1f * (i % 4), 0.2f * i);
  for (int i = 0; i < 3; ++i) AddInstance(kE, 100.f, 1, -100.f + i * 30.f, 2.f, 0.7f * i);
  for (int i = 0; i < 4; ++i) AddInstance(kD, 2000.f + i * 10.f, 0, 2000.f);  // outside the region
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    CHECK(bvh::g_pool.instances.size() == 39u, "instances %zu", bvh::g_pool.instances.size());
  }

  Stage("1 streaming: 5 presents");
  // 1. Streaming under the budget.
  present();
  std::printf("frame 1: resident %u pending %u tlas %u waiting %u reason %s\n", data->live.resident_meshes,
              data->live.pending_meshes, data->live.tlas_instances, data->live.tlas_waiting, data->live.tlas_reason);
  CHECK(data->live.resident_meshes == 1u && data->live.pending_meshes == 4u, "A alone");
  CHECK(data->live.tlas_rebuilds == 1u && std::string(data->live.tlas_reason) == "first build", "first TLAS");
  CHECK(data->live.tlas_instances == 2u && data->live.tlas_waiting == 33u, "A's instances in, others wait (%u, %u)",
        data->live.tlas_instances, data->live.tlas_waiting);
  CHECK(data->bvh_ready, "traceable after the first TLAS");
  present();
  CHECK(data->live.resident_meshes == 2u && data->live.pending_meshes == 3u, "B alone (%u, %u)", data->live.resident_meshes, data->live.pending_meshes);
  present();
  CHECK(data->live.resident_meshes == 3u && data->live.pending_meshes == 2u, "C alone");
  present();
  CHECK(data->live.resident_meshes == 4u && data->live.pending_meshes == 1u, "D; E over the remaining budget");
  present();
  CHECK(data->live.resident_meshes == 5u && data->live.pending_meshes == 0u && !data->mesh_scan_needed, "E");
  const uint32_t first_tlas = data->live.tlas_frame;
  for (int i = 0; i < 3; ++i) present();
  CHECK(data->live.tlas_rebuilds == 1u, "no rebuild inside the interval");
  present();
  CHECK(data->live.tlas_rebuilds == 2u && data->live.tlas_frame == first_tlas + bvh::kLiveTlasInterval, "rebuild after the interval (%u)", data->live.tlas_frame - first_tlas);
  CHECK(data->live.tlas_instances == 35u && data->live.tlas_waiting == 0u, "all region instances (%u)", data->live.tlas_instances);
  CHECK(data->live.tlas_instances == BuildRefInstances(*data, bvh::CurrentPoolRegion()).size(), "matches brute-force set");
  Stage("1 idle presents");
  for (int i = 0; i < 20; ++i) present();
  CHECK(data->live.tlas_rebuilds == 2u, "nothing changed, no rebuild");
  std::printf("store: %.1f MB used %.1f MB allocated, grows %u, mesh ms max %.2f tlas ms max %.2f\n",
              data->live.used_bytes / 1048576.0, data->live.capacity_bytes / 1048576.0, data->live.grows,
              data->live.mesh_ms_max, data->live.tlas_ms_max);
  CHECK(data->live.grows >= 4u && data->live.capacity_bytes >= data->live.used_bytes, "arenas grew");
  CHECK(dev.bad_updates == 0 && dev.bad_copies == 0, "GPU writes in range");
  CHECK(CompareTraces(dev, *data, "streamed", 300, 1) == 0, "trace agrees");
  {
    const int calls = renodx::utils::scene::g_readback_calls;
    CHECK(calls == 0, "no readback outside the check (%d)", calls);
    Stage("1 GPU check");
    bvh::g_live_bvh.check_requested.store(true);
    present();
    std::printf("check: %s\n", data->live.gpu_result.c_str());
    CHECK(data->live.gpu_checked && data->live.gpu_ok, "GPU check");
    CHECK(renodx::utils::scene::g_readback_calls > calls, "check reads back");
  }
  CHECK(data->uvs.buffer.handle == 0u && data->uvs.capacity == 0u && data->uvs.used == 0u, "no UV arena without UV meshes");
  for (const bvh::LiveMeshSlot& slot : data->slots) CHECK(slot.uv_count == 0u, "slot without UVs (uid %llx)", (unsigned long long)slot.uid);
  for (const bvh::WorldMeshGPU& d : data->mesh_descriptors) CHECK(d.bbox_min[3] == 0.f, "descriptor bbox_min.w is 0 without UVs");

  // 2. A new instance: TLAS rebuilt once the interval has passed.
  Stage("2 new instance");
  AddInstance(kD, 5, 30, 5, 3.f);
  present();
  CHECK(std::string(data->live.tlas_reason) == "pool changed" && data->live.tlas_instances == 36u, "new instance (%s %u)", data->live.tlas_reason, data->live.tlas_instances);

  // 3. Region moves with the camera.
  Stage("3 region move");
  SetCamera(2010.f, 0.f, 2005.f);
  for (int i = 0; i < 9; ++i) present();
  std::printf("moved: tlas %u reason %s\n", data->live.tlas_instances, data->live.tlas_reason);
  CHECK(std::string(data->live.tlas_reason) == "region moved" && data->live.tlas_instances == 4u, "far instances");
  CHECK(CompareTraces(dev, *data, "moved", 400, 2) == 0, "trace agrees after the move");
  SetCamera(10.f, 5.f, 10.f);
  for (int i = 0; i < 9; ++i) present();
  CHECK(data->live.tlas_instances == 36u, "back (%u)", data->live.tlas_instances);

  // 4. Retire B: its descriptor is zeroed at once, its instances leave the TLAS.
  const uint32_t slot_b = data->slot_by_uid.count(2u) ? data->slot_by_uid[2u] : 99u;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    CHECK(bvh::g_pool.meshes[1].uid == 2u, "uid order");
  }
  Stage("4 retire B");
  RetireMesh(kB);
  present();
  CHECK(data->live.meshes_retired == 1u && !data->slots[slot_b].live, "B retired");
  {
    const bvh::WorldMeshGPU zero = {};
    const auto meshes = MakeView<bvh::WorldMeshGPU>(dev, data->mesh_srv);
    const bvh::WorldMeshGPU gpu = meshes[slot_b];
    CHECK(std::memcmp(&gpu, &zero, sizeof(zero)) == 0, "zeroed descriptor uploaded");
  }
  for (int i = 0; i < 9; ++i) present();
  CHECK(data->live.tlas_instances == 31u, "B's 5 instances gone (%u)", data->live.tlas_instances);
  CHECK(CompareTraces(dev, *data, "retired", 300, 3) == 0, "trace agrees after retirement");
  Stage("4 GPU check after retire");
  bvh::g_live_bvh.check_requested.store(true);
  present();
  CHECK(data->live.gpu_ok, "GPU check after retirement: %s", data->live.gpu_result.c_str());

  // 5. Retire A: retired data passes half of the store -> reset and restream.
  Stage("5 retire A, reset, restream");
  RetireMesh(kA);
  present();
  std::printf("after A: resets %u reason '%s' slots %zu bvh_ready %d\n", data->live.resets, data->live.last_reset_reason.c_str(),
              data->slots.size(), (int)data->bvh_ready);
  CHECK(data->live.resets == 1u && data->live.last_reset_reason == "retired meshes fill half of the store", "reset");
  CHECK(data->slots.empty() && !data->bvh_ready, "store dropped");
  for (int i = 0; i < 12; ++i) present();
  CHECK(data->live.resident_meshes == 3u && data->live.pending_meshes == 0u, "C D E restreamed (%u)", data->live.resident_meshes);
  CHECK(data->live.garbage_bytes == 0u, "no garbage after reset");
  CHECK(data->live.tlas_instances == 29u && data->bvh_ready, "TLAS back (%u)", data->live.tlas_instances);
  CHECK(CompareTraces(dev, *data, "restreamed", 300, 4) == 0, "trace agrees after reset");

  // 6. GPU allocation failure: retried after kLiveRetryFrames.
  Stage("6 GPU alloc failure");
  const uint64_t kF = 0xF;
  AddMesh(kF, Grid(250, 250, 30.f, 1.f));
  AddInstance(kF, -100, 0, 100);
  dev.fail_gpu_creates = 1;
  present();
  CHECK(data->live.upload_failures == 1u && data->live.pending_meshes == 1u, "growth failed, F pending (%u %u)",
        data->live.upload_failures, data->live.pending_meshes);
  std::printf("failure: %s\n", data->live.last_failure.c_str());
  for (uint32_t i = 0; i + 2 < bvh::kLiveRetryFrames; ++i) present();
  CHECK(data->live.pending_meshes == 1u, "waits");
  for (int i = 0; i < 3; ++i) present();
  CHECK(data->live.pending_meshes == 0u && data->live.resident_meshes == 4u, "retried and uploaded");
  for (int i = 0; i < 9; ++i) present();
  CHECK(data->live.tlas_instances == 30u, "F's instance in (%u)", data->live.tlas_instances);

  // 7. Live BVH off: nothing changes; on again: catches up.
  Stage("7 live off/on");
  bvh::g_live_bvh.enabled.store(false);
  const uint64_t kG = 0x10;
  AddMesh(kG, BoxMesh(2.f));
  AddInstance(kG, 0, 50, 0);
  const uint32_t rebuilds = data->live.tlas_rebuilds;
  for (int i = 0; i < 20; ++i) present();
  CHECK(data->live.resident_meshes == 4u && data->live.tlas_rebuilds == rebuilds, "frozen while off");
  bvh::g_live_bvh.enabled.store(true);
  for (int i = 0; i < 9; ++i) present();
  CHECK(data->live.resident_meshes == 5u && data->live.tlas_instances == 31u, "caught up");

  // 8. A mesh without a usable triangle fails; its instances count as unusable.
  Stage("8 failed mesh");
  const uint64_t kH = 0x11;
  {
    bvh::PoolDecodedMesh broken = BoxMesh(1.f);
    for (auto& tri : broken.triangles) tri = {0u, 0u, 99u};
    AddMesh(kH, broken);
  }
  AddInstance(kH, 3, 3, 3);
  for (int i = 0; i < 9; ++i) present();
  CHECK(data->live.failed_meshes == 1u && data->live.blas_invalid == 1u && data->live.tlas_unusable == 1u, "failed mesh (%u %u %u)",
        data->live.failed_meshes, data->live.blas_invalid, data->live.tlas_unusable);
  CHECK(data->live.pending_meshes == 0u && !data->mesh_scan_needed, "failed mesh is not retried");

  // 8b. Descriptor buffer creation fails: the new slot is not traced until
  // the upload is retried; then the TLAS picks it up.
  Stage("8b descriptor failure");
  const uint64_t kI = 0x12;
  AddMesh(kI, BoxMesh(0.5f));
  AddInstance(kI, -5, 40, -5);
  {
    const int creates = dev.creates;
    const uint32_t failures = data->live.upload_failures;
    dev.fail_gpu_creates = 1;
    present();
    (void)creates;
    CHECK(data->live.upload_failures == failures + 1u && data->live.last_failure.rfind("mesh descriptor", 0) == 0,
          "descriptor creation failed");
    CHECK(data->descriptors_dirty && data->descriptor_count + 1u == data->slots.size(), "slot uploaded, descriptors behind");
    std::printf("descriptor failure: %s\n", data->live.last_failure.c_str());
    for (int i = 0; i < 9; ++i) present();
    CHECK(data->live.tlas_waiting == 1u && data->live.tlas_instances == 31u, "I waits (%u %u)", data->live.tlas_waiting, data->live.tlas_instances);
    for (uint32_t i = 0; i < bvh::kLiveRetryFrames; ++i) present();
    CHECK(!data->descriptors_dirty && data->descriptor_count == data->slots.size(), "descriptors retried");
    CHECK(data->live.tlas_instances == 32u && data->live.tlas_waiting == 0u, "I traced (%u)", data->live.tlas_instances);
  }

  // 8c. TLAS buffer creation fails: the previous TLAS stays and the build is
  // retried without any further change.
  {
    Stage("8c TLAS create failure");
    for (int i = 0; i < 9; ++i) present();
    AddInstance(kD, 7, 45, 7);
    const uint32_t failures = data->live.upload_failures;
    dev.fail_gpu_creates = 1;
    present();
    CHECK(data->live.upload_failures == failures + 1u && data->live.tlas_instances == 32u && data->bvh_ready
          && data->active_count == 32u, "previous TLAS kept (%u)", data->live.tlas_instances);
    // Traceable and consistent (invalid/overflow checked inside); the only
    // disagreements are rays at the instance the kept TLAS does not have yet.
    CompareTraces(dev, *data, "kept TLAS (one instance behind the pool)", 200, 6);
    for (uint32_t i = 0; i + 1 < bvh::kLiveRetryFrames; ++i) present();
    CHECK(data->live.tlas_instances == 32u, "waits for the retry frame");
    present();
    CHECK(data->live.tlas_instances == 33u, "retried (%u)", data->live.tlas_instances);
  }

  // 8d. Provenance, inspect description, write tracking, mass retirement.
  {
    Stage("8d provenance/inspect");
    for (int i = 0; i < 9; ++i) present();
    CHECK(!data->tlas_info.empty() && data->tlas_info.size() == data->tlas_instances_cpu.size(), "provenance per TLAS instance");
    // Pick a TLAS instance of mesh C and describe it as if the trace hit it.
    uint32_t pick = UINT32_MAX;
    for (uint32_t i = 0; i < data->tlas_info.size(); ++i) {
      if (data->tlas_info[i].mesh_key == kC) { pick = i; break; }
    }
    CHECK(pick != UINT32_MAX, "an instance of C in the TLAS");
    if (pick != UINT32_MAX) {
      const auto& info = data->tlas_info[pick];
      // (The harness observes with its own frame numbers, so only presence is checked.)
      CHECK(info.first_frame != 0u && info.admit_frame != 0u && info.vs_hash == 0x1000u, "instance provenance (%u %u)",
            info.first_frame, info.admit_frame);
      bvh::BvhInspect& inspect = data->inspect;
      inspect = {};
      inspect.valid = true;
      inspect.hit = true;
      inspect.instance = pick;
      inspect.mesh = data->slot_by_uid[info.uid];
      inspect.t = 12.5f;
      inspect.compare_class = 10u;
      bvh::DescribeLiveInspect(data);
      for (const auto& line : inspect.lines) std::printf("  inspect: %s\n", line.c_str());
      const std::string all = [&]() { std::string s; for (const auto& l : inspect.lines) s += l + "\n"; return s; }();
      CHECK(all.find("extra on sky") != std::string::npos && all.find("VS 0x00001000") != std::string::npos
            && all.find("20000 triangles") != std::string::npos && all.find("Seen in") != std::string::npos,
            "inspect lines");
      // Out-of-range instance (TLAS rebuilt since).
      inspect.instance = 100000u;
      bvh::DescribeLiveInspect(data);
      CHECK(inspect.lines.size() == 1u && inspect.lines[0].find("no longer has") != std::string::npos, "stale instance");
    }
    // Writes to a VB a mesh comes from are counted; other buffers are not.
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      bvh::AddPoolResourceKey(0x7770u, kC);
    }
    bvh::NotePoolBufferWrite(0x7770u);
    bvh::NotePoolBufferWrite(0x8880u);
    bvh::OnMapBufferRegionPool(&dev, {0x7770u}, 0, 16, map_access::read_only, nullptr);
    bvh::OnMapBufferRegionPool(&dev, {0x7770u}, 0, 16, map_access::write_discard, nullptr);
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      bvh::UpdatePoolStats();
      const auto& s = bvh::g_pool.stats;
      const auto it = bvh::g_pool.mesh_by_key.find(kC);
      const uint32_t writes = it == bvh::g_pool.mesh_by_key.end() ? 0u : bvh::g_pool.meshes[it->second].writes_after_capture;
      std::printf("writes: tracked %llu, mesh C %u, meshes written %zu\n", (unsigned long long)s.tracked_buffer_writes, writes, s.meshes_written);
      CHECK(s.tracked_buffer_writes == 2u && writes == 2u && s.meshes_written == 1u, "write tracking");
    }
    // 32 meshes retired in one compaction count as a map change.
    Stage("8d mass retirement");
    for (uint64_t k = 0; k < 32; ++k) AddMesh(0x5000u + k, BoxMesh(3.f + 0.1f * k));
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      for (uint64_t k = 0; k < 32; ++k) bvh::InvalidatePoolMeshKey(0x5000u + k);
      bvh::CompactPool();
      const auto& s = bvh::g_pool.stats;
      CHECK(s.mass_retirements == 1u && s.last_mass_retire_meshes == 32u && s.last_mass_retire_frame == frame.load(), "mass retirement");
    }
    for (int i = 0; i < 9; ++i) present();
  }

  Stage("8e HLSL interval transcription");
  // 8e. Game camera fade: GPU visibility, CPU port of the trace vs brute force.
  {
    // The HLSL transcription and camera_fade.hpp agree.
    std::mt19937 rng(7);
    std::uniform_real_distribution<float> u(-30.f, 30.f);
    int differ = 0;
    for (int i = 0; i < 20000; ++i) {
      const float start = u(rng), inv = (i % 7 == 0) ? 0.f : u(rng) * 0.05f, floor_value = (i % 11 == 0) ? 0.7f : (i % 13 == 0 ? 0.5f : 0.f);
      float a0, a1, b0, b1;
      const bool ra = HlslCameraFadeInterval(start, inv, floor_value, &a0, &a1);
      const bool rb = bvh::CameraFadeInterval(start, inv, floor_value, &b0, &b1);
      if (ra != rb || a0 != b0 || a1 != b1) ++differ;
    }
    CHECK(differ == 0, "HLSL interval transcription differs (%d)", differ);

    Stage("8e add fade meshes/instances");
    const uint64_t kF = 0xF0u, kG = 0xF1u;
    AddMesh(kF, Grid(12, 12, 6.f, 0.5f));
    AddMesh(kG, BoxMesh(1.5f));
    float cam[3]; bvh::PoolCameraPosition(cam);
    uint32_t near_fading = 0u;
    for (int i = 0; i < 12; ++i) {
      const float ang = 0.5f * i;
      const float dist = 6.f + 3.f * i;
      AddFadedInstance(kF, cam[0] + dist * std::cos(ang), cam[1] - 2.f + 0.3f * i, cam[2] + dist * std::sin(ang), 14.f, 0.5f, i % 4 != 3);
      if (i % 4 != 3) ++near_fading;
    }
    // G: three instances only drawn by a light VS (shadow-only casters), and
    // one drawn by both views whose near-fade inputs are unusable (NaN).
    for (int i = 0; i < 4; ++i) {
      float w[12];
      World(cam[0] + 9.f + 4.f * i, cam[1] + 1.f, cam[2] - 7.f, 1.f, 0.f, w);
      bvh::PoolVisibility vis;
      vis.valid = true;
      vis.color[3] = 1.f;
      vis.param[0] = 12.f;
      vis.param[1] = (i == 3) ? NAN : 1.f;
      const bvh::PoolSighting shadow{static_cast<uint8_t>(bvh::kPoolPassLight | bvh::kPoolPassVisibility), 0u};
      const bvh::PoolSighting camera{static_cast<uint8_t>(bvh::kPoolPassCamera | bvh::kPoolPassVisibility | bvh::kPoolPassNearFadePs), 0x7777u};
      std::lock_guard lock(bvh::g_pool.mutex);
      bvh::ObservePoolInstance(kG, 0x2000u, w, g_obs_frame++, &shadow, &vis);
      bvh::ObservePoolInstance(kG, 0x2000u, w, g_obs_frame++, &shadow, &vis);
      if (i == 3) bvh::ObservePoolInstance(kG, 0x2000u, w, g_obs_frame++, &camera, &vis);
    }
    Stage("8e 12 presents");
    for (int i = 0; i < 12; ++i) present();
    std::printf("fade: tlas %u near-fade %u (expected %u)\n", data->live.tlas_instances, data->live.tlas_near_fade, near_fading);
    CHECK(data->live.tlas_near_fade == near_fading, "near-fade instances in the TLAS (%u vs %u)", data->live.tlas_near_fade, near_fading);
    uint32_t gpu_fade = 0u, bad = 0u, hidden = 0u, f_seen = 0u, g_shadow_only = 0u, g_both = 0u;
    {
      const auto view = MakeView<bvh::WorldInstanceGPU>(dev, data->instance_srv);
      for (uint32_t i = 0; i < view.count; ++i) {
        const auto inst = view[i];
        const uint32_t flags = bvh::InstanceVisibilityFlags(inst);
        const uint64_t key = i < data->tlas_info.size() ? data->tlas_info[i].mesh_key : 0u;
        const bool is_f = key == kF;
        const bool is_g = key == kG;
        if ((flags & bvh::kInstanceCameraVisible) == 0u) ++hidden;
        if ((flags & bvh::kInstanceNearFade) != 0u) {
          ++gpu_fade;
          if (!(inst.visibility[0] == 14.f && inst.visibility[1] == 0.5f) || (flags & bvh::kInstanceCameraVisible) == 0u) ++bad;
        } else if (inst.visibility[0] != 0.f || inst.visibility[1] != 0.f || inst.visibility[3] != 0.f) {
          ++bad;
        }
        if (is_f) {
          ++f_seen;
          if ((flags & bvh::kInstanceCameraVisible) == 0u || (flags & bvh::kInstanceShadowCaster) != 0u) ++bad;
        } else if (is_g) {
          if (flags == bvh::kInstanceShadowCaster) ++g_shadow_only;
          else if (flags == (bvh::kInstanceShadowCaster | bvh::kInstanceCameraVisible)) ++g_both;
          else ++bad;
        } else if (flags != 0u) {
          ++bad;  // the other test instances were observed without a view
        }
      }
      CHECK(view.count == data->live.tlas_instances, "instance buffer size");
    }
    std::printf("fade flags: near %u hidden %u F %u G shadow-only %u G both %u bad %u\n", gpu_fade, hidden, f_seen, g_shadow_only, g_both, bad);
    CHECK(gpu_fade == near_fading && bad == 0u && f_seen == 12u && g_shadow_only == 3u && g_both == 1u,
          "GPU visibility flags (near %u, bad %u)", gpu_fade, bad);
    CHECK(data->live.tlas_camera_hidden == hidden, "camera-hidden stat (%u vs %u)", data->live.tlas_camera_hidden, hidden);
    {
      {
        std::lock_guard lock(falcom_world::g_state.mutex);
        falcom_world::g_state.camera.has_fade = true;
        falcom_world::g_state.camera.near_fade_floor = 0.f;
        falcom_world::g_state.camera.map_alpha = 1.f;
      }
      uint32_t pick = UINT32_MAX;
      for (uint32_t i = 0; i < data->tlas_instances_cpu.size(); ++i) {
        if ((bvh::InstanceVisibilityFlags(data->tlas_instances_cpu[i]) & bvh::kInstanceNearFade) != 0u
            && data->tlas_instances_cpu[i].visibility[0] == 14.f) { pick = i; break; }
      }
      CHECK(pick != UINT32_MAX, "a near-fading F instance");
      if (pick != UINT32_MAX) {
        bvh::BvhInspect& inspect = data->inspect;
        inspect = {};
        inspect.valid = inspect.hit = true;
        inspect.instance = pick;
        inspect.mesh = static_cast<uint32_t>(data->tlas_instances_cpu[pick].header[0]);
        inspect.t = 9.36f;
        inspect.compare_class = 10u;
        inspect.camera_hidden = true;
        bvh::DescribeLiveInspect(data);
        std::string all;
        for (const auto& line : inspect.lines) { std::printf("  inspect: %s\n", line.c_str()); all += line + "\n"; }
        CHECK(all.find("Near fade: from 14.00 m, full at 16.00 m") != std::string::npos && all.find("the game HIDES this surface") != std::string::npos
                  && all.find("2 of 2 camera draws near-fade") != std::string::npos
                  && all.find("the game camera does not show this surface (hiding is off)") != std::string::npos
                  && all.find("Views now: camera VS 2 frames") != std::string::npos
                  && all.find("drawn by camera vertex shaders (2 draws); shadow caster: not seen") != std::string::npos,
              "inspect fade lines");
        inspect.t = 30.f;
        inspect.camera_hidden = false;
        bvh::DescribeLiveInspect(data);
        all.clear();
        for (const auto& line : inspect.lines) all += line + "\n";
        CHECK(all.find("the game draws this surface") != std::string::npos, "inspect: shown beyond the fade");
      }
    }
    {
      Stage("8e visibility change");
      // A later sighting with other inputs: the TLAS is rebuilt for it alone.
      const uint32_t rebuilds = data->live.tlas_rebuilds;
      float w[12];
      const float ang = 0.f, dist = 6.f;
      World(cam[0] + dist * std::cos(ang), cam[1] - 2.f, cam[2] + dist * std::sin(ang), 1.f, 0.3f, w);
      bvh::PoolVisibility vis;
      vis.valid = true;
      vis.color[3] = 1.f;
      vis.param[0] = 20.f;
      vis.param[1] = 0.5f;
      const bvh::PoolSighting sighting{static_cast<uint8_t>(bvh::kPoolPassCamera | bvh::kPoolPassVisibility | bvh::kPoolPassNearFadePs), 0x5150u};
      {
        std::lock_guard lock(bvh::g_pool.mutex);
        bvh::ObservePoolInstance(kF, 0x1000u, w, g_obs_frame++, &sighting, &vis);
      }
      for (int i = 0; i < 9; ++i) present();
      uint32_t found = 0u;
      const auto view = MakeView<bvh::WorldInstanceGPU>(dev, data->instance_srv);
      for (uint32_t i = 0; i < view.count; ++i) found += view[i].visibility[0] == 20.f ? 1u : 0u;
      std::printf("visibility change: rebuilds %u -> %u (%s), instances with start 20: %u\n", rebuilds, data->live.tlas_rebuilds,
                  data->live.tlas_reason, found);
      CHECK(data->live.tlas_rebuilds == rebuilds + 1u && std::string(data->live.tlas_reason) == "visibility changed" && found == 1u,
            "TLAS follows a visibility change");
      for (int i = 0; i < 9; ++i) present();
      CHECK(data->live.tlas_rebuilds == rebuilds + 1u, "no further rebuild without a change");
      // Back to the original inputs for the comparisons below.
      vis.param[0] = 14.f;
      {
        std::lock_guard lock(bvh::g_pool.mutex);
        bvh::ObservePoolInstance(kF, 0x1000u, w, g_obs_frame++, &sighting, &vis);
      }
      for (int i = 0; i < 9; ++i) present();
    }
    Stage("8e fade compares");
    uint32_t faded_off = 0u, faded_on = 0u;
    CHECK(CompareFadeTraces(dev, *data, "fade shown", {false, 0.f}, 600, 11, &faded_off) == 0, "fade shown agrees");
    CHECK(CompareFadeTraces(dev, *data, "fade hidden", {true, 0.f}, 600, 11, &faded_on) == 0, "fade hidden agrees");
    CHECK(faded_off > 20u && faded_on == faded_off, "camera-faded rays counted alike (%u %u)", faded_off, faded_on);
    uint32_t faded_floor = 0u;
    CHECK(CompareFadeTraces(dev, *data, "fade floor 1", {true, 1.f}, 600, 11, &faded_floor) == 0 && faded_floor < faded_on,
          "floor 1 disables the near fade, shadow-only instances stay hidden (%u < %u)", faded_floor, faded_on);
    {
      // A shadow-only G instance inspected: never drawn by the camera.
      uint32_t g_pick = UINT32_MAX;
      for (uint32_t i = 0; i < data->tlas_instances_cpu.size(); ++i) {
        if (bvh::InstanceVisibilityFlags(data->tlas_instances_cpu[i]) == bvh::kInstanceShadowCaster) { g_pick = i; break; }
      }
      CHECK(g_pick != UINT32_MAX, "a shadow-only instance");
      if (g_pick != UINT32_MAX) {
        bvh::BvhInspect& inspect = data->inspect;
        inspect = {};
        inspect.valid = inspect.hit = true;
        inspect.instance = g_pick;
        inspect.mesh = static_cast<uint32_t>(data->tlas_instances_cpu[g_pick].header[0]);
        inspect.t = 10.f;
        inspect.camera_hidden = true;
        bvh::DescribeLiveInspect(data);
        std::string all;
        for (const auto& line : inspect.lines) { std::printf("  inspect: %s\n", line.c_str()); all += line + "\n"; }
        CHECK(all.find("NEVER drawn by a camera vertex shader (only drawn into shadow maps") != std::string::npos
                  && all.find("Views now: camera VS 0 frames") != std::string::npos && all.find("light VS (shadow maps) 2 frames") != std::string::npos,
              "inspect: shadow-only");
      }
    }
    CHECK(CompareTraces(dev, *data, "with faded instances", 300, 13) == 0, "plain trace unchanged by visibility data");
    Stage("8e GPU check");
    bvh::g_live_bvh.check_requested.store(true);
    present();
    CHECK(data->live.gpu_checked && data->live.gpu_ok, "GPU check with visibility (%s)", data->live.gpu_result.c_str());
    Stage("8e retire F,G");
    const uint64_t retires = BumpCount(bvh::PoolRevisionSource::Compact);
    RetireMesh(kF);
    RetireMesh(kG);
    for (int i = 0; i < 9; ++i) present();
    CHECK(BumpCount(bvh::PoolRevisionSource::Compact) > retires, "retired mesh bumps the Compact revision source");
    CHECK(data->live.tlas_near_fade == 0u, "near-fade instances gone with their meshes (%u)", data->live.tlas_near_fade);
  }

  Stage("8f refit");
  // 8f. Refit: a moved dynamic instance refits the TLAS without a rebuild; the
  // refit agrees with brute force; a failed refit forces a rebuild; 120 refits
  // force a rebuild for tree quality.
  {
    const uint64_t kR = 0xF8;
    AddMesh(kR, Grid(9, 9, 4.f, 0.7f));
    AddInstance(kR, 5.f, 0.f, 5.f);
    for (int i = 0; i < 12; ++i) present();
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      for (size_t i = 1; i < bvh::g_pool.instances.size(); ++i) {
        CHECK(bvh::g_pool.instances[i - 1].id < bvh::g_pool.instances[i].id, "pool ids ascending");
      }
    }
    MoveDynamicInstance(kR, 5.f, 5.f, frame.load());
    for (int i = 0; i < 10; ++i) present();
    const uint32_t rebuilds = data->live.tlas_rebuilds;
    const uint32_t refits = data->live.tlas_refits;
    const uint32_t df = data->tlas_dynamic_first;
    const std::vector<bvh::WorldInstanceGPU> prefix_instances(data->tlas_instances_cpu.begin(), data->tlas_instances_cpu.begin() + df);
    const std::vector<bvh::LiveTlasInstance> prefix_info(data->tlas_info.begin(), data->tlas_info.begin() + df);
    MoveDynamicInstance(kR, 5.4f, 5.f, frame.load());
    for (int i = 0; i < 10; ++i) present();
    CHECK(data->live.tlas_refits > refits, "moved instance refitted (%u)", data->live.tlas_refits);
    CHECK(data->live.tlas_rebuilds == rebuilds, "pose change: no rebuild (%u -> %u)", rebuilds, data->live.tlas_rebuilds);
    CHECK(df > 0u && df < data->tlas_info.size(), "dynamic tail follows the static prefix (%u of %zu)", df, data->tlas_info.size());
    {
      // The tail copies (tlas_info, instance buffer CPU copy) must sit at the pool pose, not just agree with each other.
      std::lock_guard lock(bvh::g_pool.mutex);
      for (const bvh::WorldInstance& inst : bvh::g_pool.instances) {
        if (inst.mesh_key != kR) continue;
        CHECK(std::fabs(data->tlas_info[df].matrix[3] - 5.4f) < 1e-4f && std::fabs(inst.matrix[3] - 5.4f) < 1e-4f,
              "dynamic tail matrix at the pool pose x=5.4 (tlas %.3f, pool %.3f)", data->tlas_info[df].matrix[3], inst.matrix[3]);
        CHECK(std::memcmp(data->tlas_info[df].bounds_min, inst.bounds_min, sizeof(inst.bounds_min)) == 0
                  && std::memcmp(data->tlas_info[df].bounds_max, inst.bounds_max, sizeof(inst.bounds_max)) == 0,
              "dynamic tail bounds match the pool instance");
      }
    }
    CHECK(CompareTraces(dev, *data, "refit", 60, 21) == 0, "trace agrees after refit");
    bvh::g_live_bvh.check_requested.store(true);
    present();
    CHECK(data->live.gpu_ok, "GPU check after refit: %s", data->live.gpu_result.c_str());
    CHECK(std::memcmp(prefix_instances.data(), data->tlas_instances_cpu.data(), prefix_instances.size() * sizeof(bvh::WorldInstanceGPU)) == 0,
          "static prefix of the instance copy unchanged by the refit");
    CHECK(std::memcmp(prefix_info.data(), data->tlas_info.data(), prefix_info.size() * sizeof(bvh::LiveTlasInstance)) == 0,
          "static prefix of tlas_info unchanged by the refit");
    {
      // Brute force: every descriptor, leaf bound and node recomputed from tlas_info.
      std::vector<bvh::WorldInstanceGPU> ref_instances;
      std::vector<bvh::BVHLeafGPU> ref_leaves = data->tlas_leaves_cpu;
      for (size_t i = 0; i < data->tlas_info.size(); ++i) {
        ref_instances.push_back(bvh::MakeLiveInstanceGPU(data->tlas_info[i], data->slot_by_uid.at(data->tlas_info[i].uid)));
        bvh::BVHLeafGPU& leaf = ref_leaves[data->tlas_leaf_of_instance[i]];
        for (int k = 0; k < 3; ++k) {
          leaf.bounds_min[k] = data->tlas_info[i].bounds_min[k];
          leaf.bounds_max[k] = data->tlas_info[i].bounds_max[k];
        }
      }
      std::vector<bvh::BVHNodeGPU> ref_nodes = data->tlas_nodes_cpu;
      bvh::RefitBvhNodes(ref_leaves, &ref_nodes);
      CHECK(ref_instances.size() == data->tlas_instances_cpu.size()
                && std::memcmp(ref_instances.data(), data->tlas_instances_cpu.data(), ref_instances.size() * sizeof(bvh::WorldInstanceGPU)) == 0,
            "instances agree with the brute-force recompute");
      CHECK(ref_leaves.size() == data->tlas_leaves_cpu.size()
                && std::memcmp(ref_leaves.data(), data->tlas_leaves_cpu.data(), ref_leaves.size() * sizeof(bvh::BVHLeafGPU)) == 0,
            "leaves agree with the brute-force recompute");
      CHECK(ref_nodes.size() == data->tlas_nodes_cpu.size()
                && std::memcmp(ref_nodes.data(), data->tlas_nodes_cpu.data(), ref_nodes.size() * sizeof(bvh::BVHNodeGPU)) == 0,
            "nodes agree with the brute-force recompute");
    }

    // Every instance dynamic: a rebuild puts the dynamic part first, from 0.
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      for (bvh::WorldInstance& inst : bvh::g_pool.instances) inst.dynamic = true;
      bvh::g_pool.revision += 1u;
    }
    for (int i = 0; i < 10; ++i) present();
    CHECK(data->tlas_dynamic_first == 0u, "all dynamic: dynamic part starts at 0 (%u)", data->tlas_dynamic_first);
    CHECK(CompareTraces(dev, *data, "all dynamic", 40, 22) == 0, "trace agrees with all instances dynamic");
    MoveDynamicInstance(kR, 150.f, 5.f, frame.load());
    for (int i = 0; i < 10; ++i) present();
    CHECK(CompareTraces(dev, *data, "far move", 60, 23) == 0, "trace agrees after a far move");

    // An instance removed between the snapshot and the refit: refit_missing, then a rebuild.
    const uint32_t missing = data->live.refit_missing;
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      for (auto it = bvh::g_pool.instances.begin(); it != bvh::g_pool.instances.end(); ++it) {
        if (it->mesh_key != kR) continue;
        bvh::g_pool.admitted_keys.erase(bvh::PoolAdmittedKey(it->mesh_id, falcom_world::MatrixHash(it->matrix, 12)));
        const auto observed = bvh::g_pool.observations.find(bvh::InstanceKey{kR, falcom_world::MatrixHash(it->matrix, 12)});
        if (observed != bvh::g_pool.observations.end()) observed->second.admitted = false;
        bvh::g_pool.instances.erase(it);
        break;
      }
      bvh::g_pool.dynamic_revision += 1u;
    }
    present();
    CHECK(data->live.refit_missing > missing, "missing instance counted (%u)", data->live.refit_missing);
    for (int i = 0; i < 3; ++i) present();
    CHECK(std::string(data->live.tlas_reason) == "refit failed", "rebuild after a failed refit (%s)", data->live.tlas_reason);
    CHECK(CompareTraces(dev, *data, "after missing instance", 40, 24) == 0, "trace agrees after the failed refit");

    // 120 refits since a rebuild: the next one is a rebuild for quality.
    AddMesh(0xF9, Grid(10, 10, 4.f, 0.6f));
    const uint64_t admits = BumpCount(bvh::PoolRevisionSource::Admit);
    AddInstance(0xF9, 5.f, 0.f, 5.f);
    for (int i = 0; i < 12; ++i) present();
    CHECK(BumpCount(bvh::PoolRevisionSource::Admit) > admits, "admit bumps the Admit revision source");
    MoveDynamicInstance(0xF9, 5.f, 5.f, frame.load());
    for (int i = 0; i < 130; ++i) {
      MoveDynamicInstance(0xF9, 5.f + 0.01f * static_cast<float>(i % 2), 5.f, frame.load());
      present();
    }
    CHECK(std::string(data->live.tlas_reason) == "refit quality", "refit quality rebuild (%s; refits %u, failures %u: %s)",
          data->live.tlas_reason, data->live.tlas_refits, data->live.refit_failures, data->live.last_failure.c_str());
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      const bvh::PoolTlasTiming& timing = bvh::g_pool.tlas_timing;
      CHECK(timing.rebuilds_by_cause[0] + timing.rebuilds_by_cause[1] + timing.rebuilds_by_cause[2] == timing.rebuilds,
            "rebuild causes sum to rebuilds (%llu)", static_cast<unsigned long long>(timing.rebuilds));
      const float cause_ms = timing.rebuild_ms_by_cause[0] + timing.rebuild_ms_by_cause[1] + timing.rebuild_ms_by_cause[2];
      CHECK(std::fabs(cause_ms - timing.rebuild_ms_sum) < 1e-3f, "rebuild ms by cause sum to rebuild_ms_sum (%.4f vs %.4f)", cause_ms,
            timing.rebuild_ms_sum);
    }
  }

  Stage("8f2 refit between structural changes");
  // 8f2. A pool revision bump waits out the rebuild interval; the moved dynamic
  // instance is still refitted, so its TLAS pose follows every frame.
  {
    const uint64_t kS = 0xF6;
    AddMesh(kS, Grid(9, 9, 4.f, 0.5f));
    AddInstance(kS, 5.f, 0.f, 5.f);
    for (int i = 0; i < 12; ++i) present();
    MoveDynamicInstance(kS, 5.f, 5.f, frame.load());
    for (int i = 0; i < 10; ++i) present();
    const uint32_t refits = data->live.tlas_refits;
    uint32_t stale_frames = 0u;
    for (int i = 0; i < 16; ++i) {
      const float x = 5.f + 0.03f * static_cast<float>(i + 1);
      MoveDynamicInstance(kS, x, 5.f, frame.load());
      if (i % 2 == 0) {
        std::lock_guard lock(bvh::g_pool.mutex);
        bvh::g_pool.revision += 1u;
      }
      present();
      float expected[12];
      World(x, 0.f, 5.f, 1.f, 0.f, expected);
      bool stale = true;
      for (const bvh::LiveTlasInstance& info : data->tlas_info) {
        if (info.mesh_key == kS && std::memcmp(info.matrix, expected, sizeof(expected)) == 0) stale = false;
      }
      if (stale) stale_frames += 1u;
    }
    CHECK(stale_frames == 0u, "TLAS pose follows each frame under structural changes (%u stale frames)", stale_frames);
    CHECK(data->live.tlas_refits - refits >= 12u, "refits between structural changes (%u)", data->live.tlas_refits - refits);
    CHECK(CompareTraces(dev, *data, "structural changes", 60, 26) == 0, "trace agrees after structural changes");

    {
      // No dynamic instance in the TLAS: a revision bump takes the revision over without copying tlas_info.
      std::vector<bool> dynamic_flags;
      {
        std::lock_guard lock(bvh::g_pool.mutex);
        for (bvh::WorldInstance& inst : bvh::g_pool.instances) {
          dynamic_flags.push_back(inst.dynamic);
          inst.dynamic = false;
        }
        bvh::g_pool.revision += 1u;
      }
      for (int i = 0; i < 10; ++i) present();
      CHECK(!data->tlas_info.empty() && data->tlas_dynamic_first == data->tlas_info.size(),
            "no dynamic instance in the TLAS (%u of %zu)", data->tlas_dynamic_first, data->tlas_info.size());
      const bvh::LiveTlasInstance* info_before = data->tlas_info.data();
      const uint32_t refits_before = data->live.tlas_refits;
      uint64_t dynamic_revision = 0u;
      {
        std::lock_guard lock(bvh::g_pool.mutex);
        bvh::g_pool.dynamic_revision += 1u;
        dynamic_revision = bvh::g_pool.dynamic_revision;
      }
      present();
      CHECK(data->tlas_info.data() == info_before, "revision bump with no dynamic instance: tlas_info not copied");
      CHECK(data->live.tlas_refits == refits_before, "revision bump with no dynamic instance: no refit (%u -> %u)", refits_before,
            data->live.tlas_refits);
      CHECK(data->tlas_dynamic_revision == dynamic_revision, "TLAS revision synced without a refit (%llu)",
            static_cast<unsigned long long>(data->tlas_dynamic_revision));
      {
        std::lock_guard lock(bvh::g_pool.mutex);
        size_t k = 0u;
        for (bvh::WorldInstance& inst : bvh::g_pool.instances) inst.dynamic = dynamic_flags[k++];
        bvh::g_pool.revision += 1u;
      }
      for (int i = 0; i < 10; ++i) present();

      // Two drains per revision: the first refits, the second finds the revision already in the TLAS (skipped).
      uint64_t skipped_before = 0u;
      {
        std::lock_guard lock(bvh::g_pool.mutex);
        skipped_before = bvh::g_pool.tlas_timing.refit_after_drain_skipped;
      }
      const uint32_t refits_drain = data->live.tlas_refits;
      for (int i = 0; i < 2; ++i) {
        MoveDynamicInstance(kS, 5.9f + 0.1f * static_cast<float>(i), 5.f, frame.load());
        bvh::RefitLiveTlasAfterDrain(&dev, &queue);
        bvh::RefitLiveTlasAfterDrain(&dev, &queue);
        present();
      }
      uint64_t skipped_after = 0u;
      uint64_t after_drain_runs = 0u;
      uint64_t pool_refits = 0u;
      uint64_t timing_rebuilds = 0u;
      uint32_t since_rebuild = 0u;
      double timing_sums = 0.0;
      {
        std::lock_guard lock(bvh::g_pool.mutex);
        const bvh::PoolTlasTiming& timing = bvh::g_pool.tlas_timing;
        skipped_after = timing.refit_after_drain_skipped;
        after_drain_runs = timing.refits_after_drain;
        pool_refits = timing.refits;
        timing_rebuilds = timing.rebuilds;
        since_rebuild = timing.refits_since_rebuild;
        timing_sums = static_cast<double>(timing.rebuild_ms_sum) + timing.refit_ms_sum + timing.after_drain_ms_sum;
      }
      CHECK(data->live.tlas_refits - refits_drain == 2u, "one refit per revision after drains (%u)", data->live.tlas_refits - refits_drain);
      CHECK(skipped_after - skipped_before == 2u, "covered revision counted as skipped (%llu)",
            static_cast<unsigned long long>(skipped_after - skipped_before));
      CHECK(after_drain_runs >= 2u && pool_refits == data->live.tlas_refits, "timing refit count matches live.tlas_refits (%llu, %u)",
            static_cast<unsigned long long>(pool_refits), data->live.tlas_refits);
      CHECK(timing_rebuilds == data->live.tlas_rebuilds && since_rebuild == data->tlas_refits_since_rebuild,
            "timing rebuild count and refits since rebuild match (%llu, %u)", static_cast<unsigned long long>(timing_rebuilds),
            since_rebuild);
      CHECK(timing_sums >= 0.0 && data->live.tlas_ms_last >= 0.f && data->live.refit_ms_last >= 0.f,
            "tlas timing non-negative (sums %.3f ms)", timing_sums);
      std::printf("tlas_timing: rebuilds %llu, refits %llu, after drain %llu (skipped %llu), since rebuild %u, sums %.3f ms\n",
                  static_cast<unsigned long long>(timing_rebuilds), static_cast<unsigned long long>(pool_refits),
                  static_cast<unsigned long long>(after_drain_runs), static_cast<unsigned long long>(skipped_after), since_rebuild, timing_sums);
    }
  }

  Stage("8g static instance flips to dynamic");
  // 8g. A static instance that turns dynamic through the follow path: the flip bumps
  // the revision, so the next rebuild partitions it and its TLAS entry follows its pose.
  // The P2a path (per-frame switch off): with it on, flagged meshes are left out of the TLAS (8h).
  bvh::g_pool.dynamic_per_frame.store(false);
  {
    const uint64_t kF = 0xF7;
    AddMesh(kF, Grid(8, 8, 4.f, 0.6f));
    AddInstance(kF, 5.f, 0.f, 5.f);
    AddInstance(kF, -4.f, 0.f, 3.f);
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      bvh::g_pool.meshes[bvh::g_pool.mesh_by_key.at(kF)].dynamic = true;
    }
    for (int i = 0; i < 12; ++i) present();
    FollowInstance(kF, -4.f, 3.f, -4.5f, 3.f, frame.load());
    for (int i = 0; i < 10; ++i) present();
    FollowInstance(kF, 5.f, 5.f, 5.6f, 5.f, frame.load());
    for (int i = 0; i < 12; ++i) present();
    CHECK(CompareTraces(dev, *data, "static instance flipped", 80, 25) == 0, "trace agrees after the flip");
    bvh::g_live_bvh.check_requested.store(true);
    present();
    CHECK(data->live.gpu_ok, "GPU check after the flip: %s", data->live.gpu_result.c_str());
  }
  bvh::g_pool.dynamic_per_frame.store(true);

  Stage("8h per-frame: a flagged mesh is left out of the TLAS; the switch off puts it back");
  // 8h. P2b (default on): a flagged mesh's instances are drawn per frame, not traced from the TLAS.
  {
    const uint64_t kG = 0xFA;
    AddMesh(kG, Grid(7, 9, 4.f, 0.6f));  // distinct content: identical meshes share one slot
    AddInstance(kG, 5.f, 0.f, 5.f);
    AddInstance(kG, -4.f, 0.f, 3.f);
    // kH: an alpha-tested flagged mesh with a slice (ready). Its draws are never recorded, so it stays in the TLAS.
    const uint64_t kH = 0xFB;
    bvh::PoolDecodedMesh alpha_mesh = Grid(6, 6, 4.f, 0.5f);
    alpha_mesh.uvs.assign(alpha_mesh.positions.size(), {0.f, 0.f});
    AddMesh(kH, alpha_mesh);
    uint64_t uid = 0u;
    uint64_t alpha_uid = 0u;
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      const uint32_t mesh_id = bvh::g_pool.mesh_by_key.at(kG);
      bvh::g_pool.meshes[mesh_id].dynamic = true;
      bvh::g_pool.dynamic_flag_revision += 1u;
      uid = bvh::g_pool.meshes[mesh_id].uid;
      const uint32_t alpha_id = bvh::g_pool.mesh_by_key.at(kH);
      bvh::MarkPoolMeshAlpha(alpha_id, bvh::g_pool.alpha_keys[kH]);
      bvh::g_pool.meshes[alpha_id].dynamic = true;
      bvh::g_pool.dynamic_flag_revision += 1u;
      alpha_uid = bvh::g_pool.meshes[alpha_id].uid;
    }
    AddInstance(kH, 2.f, 0.f, -6.f);
    AddInstance(kH, -6.f, 0.f, -2.f);
    const int32_t slice = data->alpha.slices.Acquire(alpha_uid);
    CHECK(slice >= 0, "alpha stage: a slice for the flagged alpha mesh");
    data->alpha.slot_of_uid[alpha_uid] = static_cast<uint32_t>(slice);
    const auto in_tlas = [&](uint64_t mesh_uid) {
      size_t count = 0u;
      for (const auto& info : data->tlas_info) count += info.uid == mesh_uid ? 1u : 0u;
      return count;
    };
    const auto not_ready_logs = [&]() {
      std::lock_guard lock(renodx::utils::log::g_lines_mutex);
      size_t count = 0u;
      for (const auto& line : renodx::utils::log::g_lines) count += line.second.find("GPU objects not ready") != std::string::npos ? 1u : 0u;
      return count;
    };
    // Not ready (no GPU objects, as the harness never runs the expansion): the switch on keeps the flagged mesh in the TLAS.
    data->dynamic.not_ready_logged = false;
    const size_t logs_before = not_ready_logs();
    for (int i = 0; i < 3; ++i) present();
    CHECK(in_tlas(uid) == 2u, "P2b not ready: flagged mesh stays in the TLAS (%zu)", in_tlas(uid));
    CHECK(not_ready_logs() == logs_before + 1u, "P2b not ready: logged once over 3 presents (%zu)", not_ready_logs() - logs_before);
    data->dynamic.ready = true;
    present();
    CHECK(in_tlas(uid) == 0u, "P2b ready: flagged mesh left out of the TLAS (%zu)", in_tlas(uid));
    CHECK(in_tlas(alpha_uid) == 2u, "P2b ready: flagged alpha mesh stays in the TLAS (%zu)", in_tlas(alpha_uid));
    {
      // The flagged, non-alpha instances of the region (earlier stages may leave more than kG's two).
      std::lock_guard lock(bvh::g_pool.mutex);
      uint32_t flagged = 0u;
      for (const auto& inst : bvh::g_pool.instances) {
        if (!bvh::PoolInstanceInRegion(inst, bvh::CurrentPoolRegion())) continue;
        const auto& mesh = bvh::g_pool.meshes[inst.mesh_id];
        flagged += (mesh.dynamic && !mesh.alpha) ? 1u : 0u;
      }
      CHECK(bvh::g_pool.stats.dynamic_tlas_p2b_skipped == flagged && flagged >= 2u,
            "P2b: the TLAS build left out the %u flagged instances (stats %u)", flagged, bvh::g_pool.stats.dynamic_tlas_p2b_skipped);
      CHECK(bvh::g_pool.stats.dynamic_tlas_per_frame == 1u, "P2b: stats report the per-frame TLAS");
    }
    present();
    CHECK(not_ready_logs() == logs_before + 1u, "P2b ready: no further not-ready log (%zu)", not_ready_logs() - logs_before);
    bvh::g_pool.dynamic_per_frame.store(false);
    present();
    CHECK(in_tlas(uid) == 2u, "switch off at once: P2a path keeps its instances (%zu)", in_tlas(uid));
    bvh::g_pool.dynamic_per_frame.store(true);
    present();
    CHECK(in_tlas(uid) == 0u, "switch on at once: left out again (%zu)", in_tlas(uid));
    CHECK(in_tlas(alpha_uid) == 2u, "switch on at once: alpha mesh still in the TLAS (%zu)", in_tlas(alpha_uid));
    CHECK(CompareTraces(dev, *data, "per-frame", 40, 22) == 0, "trace agrees with the per-frame switch");
    // P2b records of the flagged mesh (left out of the TLAS): with them the trace agrees with brute force, without them it does not.
    std::vector<bvh::WorldInstanceGPU> records = BuildDynamicRecords(*data, bvh::CurrentPoolRegion());
    CHECK(records.size() >= 2u, "P2b records: one per flagged instance (%zu)", records.size());
    // A record the GPU marked invalid (header.w 0) is skipped by the trace: it changes no hit.
    bvh::WorldInstanceGPU dead = records.front();
    dead.header[3] = 0.f;
    records.push_back(dead);
    CHECK(CompareTraces(dev, *data, "P2b records", 300, 23, &records) == 0, "trace with the P2b records agrees with brute force");
    CHECK(CompareTraces(dev, *data, "no P2b records", 300, 23) > 0, "without the P2b records the flagged mesh is missing from the trace");
    // Inspect of a P2b record hit: the per-frame line (the pool id stays unknown), not the "no longer has" text.
    {
      bvh::BvhInspect& inspect = data->inspect;
      inspect.valid = true;
      inspect.hit = true;
      inspect.x = 1u;
      inspect.y = 1u;
      inspect.instance = 0x40000005u;
      bvh::DescribeLiveInspect(data);
      std::string text;
      for (const auto& line : inspect.lines) text += line + "\n";
      CHECK(text.find("per-frame record 5") != std::string::npos && text.find("no longer has") == std::string::npos,
            "P2b record inspect line: %s", text.c_str());
      inspect.instance = 0x80000001u;
      bvh::DescribeLiveInspect(data);
      CHECK(inspect.lines.size() > 0 && inspect.lines.back().find("no longer has") != std::string::npos,
            "dynamic-object inspect instance keeps its text");
      inspect.valid = false;
      inspect.hit = false;
    }
    // alpha_foliage OFF drops the alpha instances at once, whatever the per-frame rule says.
    bvh::g_pool.alpha_foliage.store(false);
    present();
    CHECK(in_tlas(alpha_uid) == 0u, "alpha_foliage off: the flagged alpha mesh is dropped (%zu)", in_tlas(alpha_uid));
    bvh::g_pool.alpha_foliage.store(true);
    // Destroy clears ready: the flagged mesh goes back into the TLAS, and the not-ready log is armed again.
    bvh::DestroyDynamicExpand(&dev, data);
    present();
    CHECK(in_tlas(uid) == 2u, "P2b destroy: instances return to the TLAS (%zu)", in_tlas(uid));
    CHECK(not_ready_logs() == logs_before + 2u, "P2b destroy: not-ready logged again after re-arm (%zu)", not_ready_logs() - logs_before);
  }

  Stage("8i flag flips coalesce to the rebuild interval; the switch stays immediate");
  {
    // A flag that flips every present rebuilds the TLAS once per kLiveTlasInterval presents, not once per present.
    const uint64_t kF = 0xFC;
    AddMesh(kF, Grid(5, 5, 4.f, 0.7f));
    AddInstance(kF, 1.f, 0.f, 8.f);
    uint32_t mesh_id = 0u;
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      mesh_id = bvh::g_pool.mesh_by_key.at(kF);
    }
    data->dynamic.ready = true;
    present();  // the readiness switch is applied at once
    const uint32_t settled = data->live.tlas_rebuilds;
    for (int i = 0; i < 24; ++i) {
      {
        std::lock_guard lock(bvh::g_pool.mutex);
        bvh::g_pool.meshes[mesh_id].dynamic = (i % 2) == 0;
        bvh::g_pool.dynamic_flag_revision += 1u;
      }
      present();
    }
    const uint32_t flips = data->live.tlas_rebuilds - settled;
    CHECK(flips <= 24u / bvh::kLiveTlasInterval + 1u, "flag flips: %u rebuilds over 24 presents (limit %u)", flips, 24u / bvh::kLiveTlasInterval + 1u);
    const uint32_t before_off = data->live.tlas_rebuilds;
    bvh::g_pool.dynamic_per_frame.store(false);
    present();
    CHECK(data->live.tlas_rebuilds == before_off + 1u, "switch off: rebuilt on the next present (%u)", data->live.tlas_rebuilds - before_off);
    const uint32_t before_on = data->live.tlas_rebuilds;
    bvh::g_pool.dynamic_per_frame.store(true);
    present();
    CHECK(data->live.tlas_rebuilds == before_on + 1u, "switch on: rebuilt on the next present (%u)", data->live.tlas_rebuilds - before_on);
    bvh::DestroyDynamicExpand(&dev, data);
    RetireMesh(0xFC);
  }

  RetireMesh(0xF6);
  RetireMesh(0xF7);
  RetireMesh(0xF8);
  RetireMesh(0xF9);
  RetireMesh(0xFA);
  RetireMesh(0xFB);
  for (int i = 0; i < 9; ++i) present();

  Stage("9 requested reset");
  // 9. Rebuild on request, also inside an upload retry window.
  data->upload_retry_frame = frame.load() + 100u;
  bvh::g_live_bvh.reset_requested.store(true);
  present();
  CHECK(data->live.resets == 2u && data->live.last_reset_reason == "requested", "requested reset");
  CHECK(data->upload_retry_frame == 0u, "retry window cleared");
  for (int i = 0; i < 12; ++i) present();
  CHECK(data->live.resident_meshes == 6u && data->bvh_ready, "restreamed after request (%u)", data->live.resident_meshes);
  CHECK(CompareTraces(dev, *data, "rebuilt", 300, 5) == 0, "trace agrees after rebuild");
  Stage("9 GPU check after rebuild");
  bvh::g_live_bvh.check_requested.store(true);
  present();
  CHECK(data->live.gpu_ok, "GPU check after rebuild: %s", data->live.gpu_result.c_str());

  Stage("10 corruption check");
  // 10. GPU check catches a corrupted node.
  {
    const bvh::LiveMeshSlot& slot = data->slots.back();
    Res& nodes = dev.res[data->blas_nodes.buffer.handle];
    nodes.bytes[static_cast<size_t>(slot.node_offset) * sizeof(bvh::BVHNodeGPU) + 2] ^= 0x40;
    bvh::g_live_bvh.check_requested.store(true);
    present();
    std::printf("corrupted: %s\n", data->live.gpu_result.c_str());
    CHECK(!data->live.gpu_ok, "corruption detected");
    nodes.bytes[static_cast<size_t>(slot.node_offset) * sizeof(bvh::BVHNodeGPU) + 2] ^= 0x40;
  }

  // 11. Pool reset: no live mesh left -> store reset, BVH not traceable.
  Stage("11 pool reset");
  bvh::ResetWorldPool();
  present();
  CHECK(data->live.resets == 3u && data->live.last_reset_reason == "no live mesh left", "pool reset (%s)", data->live.last_reset_reason.c_str());
  for (int i = 0; i < 9; ++i) present();
  CHECK(!data->bvh_ready && data->live.tlas_instances == 0u, "empty BVH");

  // 11b. UV arena (alpha foliage): a mesh with UVs gets them in the arena and bbox_min.w = uv_offset + 1;
  // a mesh without UVs keeps bbox_min.w = 0 and adds nothing to the arena. The GPU check reads the arena back.
  Stage("11b UV arena");
  {
    bvh::PoolDecodedMesh uv_mesh = BoxMesh(1.f);
    for (const auto& p : uv_mesh.positions) uv_mesh.uvs.push_back({p[1] * 0.5f + 0.5f, p[2] * 0.5f + 0.5f});
    AddMesh(0x1D, uv_mesh);
    AddMesh(0x1E, BoxMesh(2.f));
    present();
    CHECK(data->slots.size() == 2u && data->uvs.used == uv_mesh.uvs.size(), "UV arena holds the UV mesh only (%zu, %llu)", data->slots.size(),
          (unsigned long long)data->uvs.used);
    const bvh::LiveMeshSlot* uv_slot = nullptr;
    size_t uv_index = 0u;
    for (size_t i = 0; i < data->slots.size(); ++i) {
      if (data->slots[i].uv_count != 0u) { uv_slot = &data->slots[i]; uv_index = i; }
    }
    CHECK(uv_slot != nullptr && uv_slot->uv_count == uv_mesh.uvs.size(), "UV slot has one UV per vertex");
    if (uv_slot != nullptr) {
      CHECK(data->mesh_descriptors[uv_index].bbox_min[3] == static_cast<float>(uv_slot->uv_offset + 1u), "descriptor bbox_min.w = uv_offset + 1");
      CHECK(data->mesh_descriptors[1 - uv_index].bbox_min[3] == 0.f, "mesh without UVs: bbox_min.w 0");
      const Res& uvs = dev.res[data->uvs.buffer.handle];
      CHECK(std::memcmp(uvs.bytes.data() + static_cast<size_t>(uv_slot->uv_offset) * 8u, uv_mesh.uvs.data(), uv_mesh.uvs.size() * 8u) == 0,
            "UV bytes in the arena match");
    }
    CHECK(data->live.used_bytes == data->vertices.used * 16u + data->indices.used * 4u + data->blas_leaves.used * sizeof(bvh::BVHLeafGPU)
          + data->blas_nodes.used * sizeof(bvh::BVHNodeGPU) + data->uvs.used * 8u, "used bytes count the UV arena");
    bvh::g_live_bvh.check_requested.store(true);
    present();
    CHECK(data->live.gpu_ok, "GPU check with UVs: %s", data->live.gpu_result.c_str());
    bvh::ResetWorldPool();
    present();
    CHECK(data->uvs.used == 0u && data->live.resident_meshes == 0u, "UV arena released with the pool");
  }

  Stage("11c alpha cut and blit (CPU)");
  TestAlphaCut();
  TestBlitPacking();
  Stage("11d cache diagnostics");
  TestCacheTiers();
  TestCacheDiff();
  TestCacheEpisodes(&dev);
  TestCacheForcedDump(&dev);
  TestCacheLines();
  TestCacheBucket();
  TestPoolResightTracker();
  TestCacheResight();
  TestCacheTierBuckets();
  TestCacheEpisodeWork();
  TestCacheCaptureTime(&dev);
  TestCacheDiagJson(&dev);
  TestDiagLabel();
  TestDiagSeparateFiles(&dev);
  TestDiagSwitches(&dev);
  TestDiagGpuAndErrors(&dev);
  TestCacheBlasWhileOff(&dev);

  TestCacheOnOff();
  TestCacheEnteredIds();
  TestCacheLatencyBuckets();
  TestLiveSnapshotMatches();
  TestCacheRebuildDiag();
  TestSkipNoop();
  TestSkipDifferent();
  TestSkipDynamicPose();
  TestSkipQuality();
  TestSkipCadence();
  TestSlackLayout();
  TestSlackApply();
  TestSlackExhaustion();
  TestSlackStructural();
  TestSlackRegionMasked();
  TestSlackToggle();
  TestSlackMaintenance();
  TestSlackSwitchSkip();
  TestSlackAppliesDiag();
  TestPerfRefusals(&dev);
  TestPerfWindow(&dev);
  TestPerfAbort(&dev);
  TestPerfSwitchesStable(&dev);
  Stage("end checks");
  CHECK(g_under_lock == 0, "graphics calls under the pool lock: %d", g_under_lock);
  std::printf("whole-buffer updates %d, copies between different strides %d\n", dev.whole_buffer_updates, dev.structured_mismatch_copies);
  CHECK(dev.structured_mismatch_copies == 0, "GPU copies keep the element stride");

  // 12. Device teardown releases every buffer and view.
  Stage("12 teardown");
  bvh::DestroyBvhDeviceData(&dev);
  CHECK(dev.res.empty() && dev.views.empty(), "leaks: %zu resources, %zu views", dev.res.size(), dev.views.size());

  std::printf("ref cache: %d hit(s), %d miss(es)\n", g_cache_hits, g_cache_misses);
  Stage("done: %d failures", g_failures);
  std::printf(g_failures == 0 ? "PASS (0 failures)\n" : "FAILED (%d failures)\n", g_failures);
  return g_failures == 0 ? 0 : 1;
}
