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
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <random>
#include <set>
#include <string>
#include <thread>
#include <Windows.h>
#include <d3d12.h>
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
  bvh::g_pool.cpu_capture_frame_us.store(2500u);
  bvh::UpdateCacheDiag(dev);
  {
    std::lock_guard<std::mutex> lock(bvh::g_cache_diag.mutex);
    CHECK(bvh::g_cache_diag.data.work.capture_ms == 2.5f && bvh::g_pool.cpu_capture_frame_us.load() == 2500u,
          "2500 us of capture CPU (the drain's frame value) gives 2.5 ms (%.3f ms)", (double)bvh::g_cache_diag.data.work.capture_ms);
  }
  bvh::g_pool.cpu_capture_frame_us.store(0u);
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
                  + ", \"refit_lag_max\": " + std::to_string(live.refit_lag_max)
                  + ", \"refit_rest_fast\": " + std::to_string(live.refit_rest_fast) + ", \"refit_rest_slow\": " + std::to_string(live.refit_rest_slow)) != std::string::npos,
        "live block has tlas_frame, refit lag and the rest counts");
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
  bvh::UpdatePerfRun(data, 5000u, base, sw);
  CHECK(!bvh::g_perf.active.load() && PerfResult().find("refused: the pool is refilling") != std::string::npos,
        "refused during the refill (%s)", PerfResult().c_str());
  bvh::g_pool.episode_start_frame = 0u;
  bvh::g_pool.map_change_frame = 4000u;
  bvh::RequestPerfRun("b");
  bvh::UpdatePerfRun(data, 5000u, base, sw);
  CHECK(!bvh::g_perf.active.load() && PerfResult().find("refused: a map change is being swept") != std::string::npos,
        "refused while a map change is swept (%s)", PerfResult().c_str());
  bvh::g_pool.episode_start_frame = 4600u;  // 400 presents after the episode start
  bvh::g_pool.map_change_frame = 0u;
  bvh::RequestPerfRun("c");
  bvh::UpdatePerfRun(data, 5000u, base, sw);
  CHECK(bvh::g_perf.active.load() && PerfResult().find("started") != std::string::npos, "allowed 400 presents after the episode start");
  const uint32_t start_frame = bvh::g_perf.run.start_frame;
  bvh::RequestPerfRun("d");
  bvh::UpdatePerfRun(data, 5001u, base + std::chrono::milliseconds(16), sw);
  CHECK(bvh::g_perf.active.load() && bvh::g_perf.run.start_frame == start_frame && PerfResult().find("already active") != std::string::npos,
        "refused while active, the run is unchanged (%s)", PerfResult().c_str());
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    bvh::g_pool.episode_serial += 1u;
  }
  bvh::UpdatePerfRun(data, 5002u, base + std::chrono::milliseconds(32), sw);
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    bvh::g_pool.episode_serial = saved_serial;
    bvh::g_pool.episode_start_frame = saved_start;
    bvh::g_pool.map_change_frame = saved_map;
  }
  CHECK(!bvh::g_perf.active.load(), "the run ended");
  bvh::ResetCacheDiagCounters();
}

// Position of `needle` at or after the first `anchor` in the text (npos when either is absent).
static size_t FindAfter(const std::string& text, const char* anchor, const std::string& needle) {
  const size_t from = text.find(anchor);
  return from == std::string::npos ? std::string::npos : text.find(needle, from);
}

// The "value" of the first `key` at or after `anchor` (NaN when absent).
static double ValueAt(const std::string& text, const char* anchor, const char* key) {
  const size_t at = FindAfter(text, anchor, key);
  if (at == std::string::npos) return std::numeric_limits<double>::quiet_NaN();
  return std::strtod(text.c_str() + text.find("\"value\": ", at) + 9, nullptr);
}

// 30 s of presents at 16 ms: the window ends at the first present with elapsed >= 30 s (1875 intervals).
// spike_at >= 0: present spike_at takes a 24 ms stall (40 ms interval, then 16 ms again) and a 9.5 ms rebuild.
// step_ms: the interval of each present (16 by default; 3 gives about 10000 presents in 30 s).
static std::string RunPerfWindow(bvh::BvhDeviceData* data, uint32_t first_frame, bool collect, int* presents, int spike_at = -1, int step_ms = 16) {
  bvh::ClearPerfGathered(data);  // the sums (rebuild, BLAS, skip, apply, blocks) start at zero, whatever ran before
  const auto base = std::chrono::steady_clock::now();
  const bvh::CacheSwitches sw{collect, false, false, true};
  bvh::g_cache_diag.enabled.store(collect);
  bvh::RequestPerfRun("window");
  bvh::UpdatePerfRun(data, first_frame, base, sw);
  int k = 0;
  while (bvh::g_perf.active.load() && k < 12000) {
    ++k;
    PerfPresentCosts(data);
    if (k == spike_at) {
      std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
      bvh::g_pool.tlas_timing.rebuild_ms_sum += 9.5f;
    }
    bvh::UpdatePerfRun(data, first_frame + static_cast<uint32_t>(k), base + std::chrono::milliseconds(step_ms * k + (spike_at >= 0 && k >= spike_at ? 24 : 0)), sw);
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
    for (const char* row : {"\"compact_observations\"", "\"compact_instances\"", "\"compact_rearm\"", "\"orphan_scan\"", "\"orphan_newest\""}) {
      CHECK(text.find(row) != std::string::npos, "perf file has the subset row %s", row);
    }
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
    CHECK(text.find("\"perf_gather\": true") != std::string::npos, "perf file says perf_gather true");
    CHECK(text.find("\"cpu_total\": {\"avg_per_present\": {\"value\": 1.875,") != std::string::npos, "CPU total is the sum of the rows (1.875 ms)");
    CHECK(text.find("\"unattributed_cpu\": {\"avg_per_present\": {\"value\": 0,") != std::string::npos, "no bracket time: unattributed clamps to 0");
    CHECK(text.find("\"gpu_total\": {\"avg_per_measured_frame\": {\"value\": 2,") != std::string::npos, "GPU total is the trace average (2 ms)");
    CHECK(CountOf(text, "\"value\": ") == CountOf(text, "\"unit\": "), "every value object has a unit");
    CHECK(text.find("\"frame_time\": {\"avg\": {\"value\": 16, \"unit\": \"ms\"}, \"avg_fps\": {\"value\": 62.5,") != std::string::npos
              && text.find("\"max\": {\"value\": 16,") != std::string::npos && text.find("\"p99\": {\"value\": 16,") != std::string::npos
              && text.find("\"hitches_over_25ms\": {\"value\": 0,") != std::string::npos && text.find("\"hitches_over_33_3ms\": {\"value\": 0,") != std::string::npos,
          "frame time max and p99 are 16 ms, no hitch");
    CHECK(FindAfter(text, "\"tlas_rebuild\"", "\"max_per_present\": {\"value\": 0.5, ") != std::string::npos, "rebuild maximum per present 0.5 ms (observed: %s)",
          text.substr(text.find("\"tlas_rebuild\""), 140).c_str());
    CHECK(std::fabs(ValueAt(text, "\"cpu_total\"", "\"max_per_present\"") - 1.875) <= 0.005, "CPU total maximum per present 1.875 ms (+-0.005)");
    CHECK(text.find("\"cpu_total\": {\"avg_per_present\": {\"value\": 1.875, \"unit\": \"ms/present\"}, \"max_per_present\": {\"value\": 1.87") != std::string::npos,
          "CPU total maximum per present about 1.875 ms (real timers add a few us)");
    CHECK(text.find("\"gpu_total\": {\"avg_per_measured_frame\": {\"value\": 2, \"unit\": \"ms/measured frame\"}, \"max_per_measured_frame\": {\"value\": 2,")
              != std::string::npos, "GPU total maximum per measured frame 2 ms");
    CHECK(text.find("\"series\": {\"capacity\": 8192, \"length\": 1875,") != std::string::npos, "series length equals the presents (1875)");
    CHECK(text.find("\"tlas_dynamic_instances\": {\"avg\"") != std::string::npos, "dynamic instance gauge written");
  }
  CHECK(path_off != path_on, "the two runs write two files");
  bvh::ResetCacheDiagCounters();
}

// One 40 ms present: the frame maximum is 40 ms, one hitch over 25 ms and one over 33.3 ms, the rebuild maximum is that present's
// 10 ms (0.5 + 9.5), CPU total maximum 11.375 ms, and the series has one value per present (1874: the window ends at the first
// present past 30 s, 16 ms x k + 24 ms stall).
static void TestPerfSpike(Device* dev) {
  bvh::ResetCacheDiagCounters();
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(dev);
  uint32_t saved_start = 0u;
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    saved_start = bvh::g_pool.episode_start_frame;
    bvh::g_pool.episode_start_frame = 0u;
  }
  int presents = 0;
  const std::string result = RunPerfWindow(data, 9000u, true, &presents, 500);
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    bvh::g_pool.episode_start_frame = saved_start;
  }
  const std::string text = ReadDumpText(WrittenPath(result));
  CHECK(JsonError(text).empty(), "spike perf file is valid JSON: %s", JsonError(text).c_str());
  CHECK(presents == 1874 && text.find("\"series\": {\"capacity\": 8192, \"length\": 1874,") != std::string::npos,
        "series has one value per present (%d presents)", presents);
  CHECK(text.find("\"frame_time\": {\"avg\"") != std::string::npos && text.find("\"max\": {\"value\": 40,") != std::string::npos,
        "frame time maximum is the 40 ms present");
  CHECK(text.find("\"hitches_over_25ms\": {\"value\": 1,") != std::string::npos && text.find("\"hitches_over_33_3ms\": {\"value\": 1,") != std::string::npos,
        "one hitch over 25 ms and one over 33.3 ms");
  CHECK(text.find("\"p99\": {\"value\": 16,") != std::string::npos, "p99 is 16 ms (one present of 40 ms in 1874)");
  CHECK(FindAfter(text, "\"tlas_rebuild\"", "\"max_per_present\": {\"value\": 10, ") != std::string::npos, "rebuild maximum per present is the 10 ms present");
  CHECK(std::fabs(ValueAt(text, "\"cpu_total\"", "\"max_per_present\"") - 11.375) <= 0.005, "CPU total maximum is the spike present total (11.375 ms, +-0.005)");
  CHECK(text.find("\"unattributed_cpu\": {\"avg_per_present\": {\"value\": 0, \"unit\": \"ms/present\"}, \"max_per_present\": {\"value\": 0,") != std::string::npos,
        "unattributed maximum is the clamped 0");
  CHECK(CountOf(text, "\"value\": ") == CountOf(text, "\"unit\": "), "every value object has a unit (spike)");
  bvh::ResetCacheDiagCounters();
}

// Whole window past the series cap: 3 ms presents (about 10000 in 30 s), a 24 ms stall and a 9.5 ms rebuild at present 9000.
// The series keeps the first 8192 presents (so its p99 is the 3 ms of those), the frame maximum is the 27 ms present, one hitch
// over 25 ms, and the rebuild maximum is the spike present's 10 ms (over the whole window).
static void TestPerfSeriesCap(Device* dev) {
  bvh::ResetCacheDiagCounters();
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(dev);
  uint32_t saved_start = 0u;
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    saved_start = bvh::g_pool.episode_start_frame;
    bvh::g_pool.episode_start_frame = 0u;
  }
  int presents = 0;
  const std::string result = RunPerfWindow(data, 9000u, true, &presents, 9000, 3);
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    bvh::g_pool.episode_start_frame = saved_start;
  }
  const std::string text = ReadDumpText(WrittenPath(result));
  CHECK(JsonError(text).empty(), "series cap perf file is valid JSON: %s", JsonError(text).c_str());
  CHECK(presents == 9992, "window ends past the series cap (%d presents)", presents);
  CHECK(text.find("\"series\": {\"capacity\": 8192, \"length\": 8192,") != std::string::npos, "series keeps the first 8192 presents");
  CHECK(text.find("\"p99\": {\"value\": 3,") != std::string::npos, "p99 over the series is 3 ms (the stall is after the cap)");
  CHECK(text.find("\"max\": {\"value\": 27,") != std::string::npos, "frame maximum is the 27 ms present (3 ms + 24 ms stall)");
  CHECK(text.find("\"hitches_over_25ms\": {\"value\": 1,") != std::string::npos, "one hitch over 25 ms");
  CHECK(FindAfter(text, "\"tlas_rebuild\"", "\"max_per_present\": {\"value\": 10, ") != std::string::npos, "rebuild maximum is the spike present's 10 ms");
  CHECK(CountOf(text, "\"value\": ") == CountOf(text, "\"unit\": "), "every value object has a unit (series cap)");
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
  bvh::UpdatePerfRun(data, 5000u, base, sw);
  for (int k = 1; k <= 3; ++k) {
    PerfPresentCosts(data);
    bvh::UpdatePerfRun(data, 5000u + static_cast<uint32_t>(k), base + std::chrono::milliseconds(16 * k), sw);
  }
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    bvh::g_pool.episode_serial += 1u;
  }
  bvh::UpdatePerfRun(data, 5004u, base + std::chrono::milliseconds(64), sw);
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
  bvh::UpdatePerfRun(data, 5000u, base, off);
  PerfPresentCosts(data);
  bvh::UpdatePerfRun(data, 5001u, base + std::chrono::milliseconds(16), slack);
  PerfPresentCosts(data);
  data->live.blas_builds = 7u;
  bvh::g_cache_diag.reset_requested.store(true);
  bvh::UpdateCacheDiag(dev);
  CHECK(data->live.blas_builds == 7u, "reset ignored while a run is active (builds %llu)", (unsigned long long)data->live.blas_builds);
  CHECK(PerfResult().find("Reset ignored") != std::string::npos, "reset ignored: result line says so");
  bvh::UpdatePerfRun(data, 5002u, base + std::chrono::milliseconds(32), off);
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    bvh::g_pool.episode_serial += 1u;
  }
  bvh::UpdatePerfRun(data, 5003u, base + std::chrono::milliseconds(48), off);
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
    bvh::SamplePerfPresent(&dev);
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
  bvh::g_perf_gate.gather.store(false);
  const CacheRun gather_off = RunCacheScenario(true);
  bvh::g_perf_gate.gather.store(true);
  CHECK(CacheRunFingerprint(gather_off) == 0x55593b70e070e7feull, "cache fingerprint with gathering OFF (the default) equals HEAD pin (%016llx)", (unsigned long long)CacheRunFingerprint(gather_off));
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
    bool full = false;
    const bool static_match = bvh::LiveSnapshotMatchesTlas(data, g, i, &full);
    return dynamic_poses ? full : static_match;
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
    bvh::SamplePerfPresent(&dev);
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
    bvh::SamplePerfPresent(&dev);
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

// Golden for the TLAS refit path: a scripted run over 12 dynamic and 6 static instances on 3 meshes.
// After every present the TLAS copies, the leaf map, tlas_info and the refit counters are hashed and
// folded into one value. The pin is the HEAD value; any change in refit output moves it.
static void MoveRefitInstance(uint64_t id, float x, float z, uint32_t frame) {
  float w[12];
  World(x, 0.f, z, 1.f, 0.f, w);
  std::lock_guard lock(bvh::g_pool.mutex);
  for (bvh::WorldInstance& inst : bvh::g_pool.instances) {
    if (inst.id != id) continue;
    float mn[3], mx[3];
    bvh::ComputePoolInstanceBounds(bvh::g_pool.meshes[inst.mesh_id], w, mn, mx);
    bvh::g_pool.admitted_keys.erase(bvh::PoolAdmittedKey(inst.mesh_id, falcom_world::MatrixHash(inst.matrix, 12)));
    std::memcpy(inst.matrix, w, sizeof(w));
    falcom_world::ComputeMatrixInverse(inst.matrix, inst.inverse_world);
    std::memcpy(inst.bounds_min, mn, sizeof(mn));
    std::memcpy(inst.bounds_max, mx, sizeof(mx));
    bvh::g_pool.admitted_keys.insert(bvh::PoolAdmittedKey(inst.mesh_id, falcom_world::MatrixHash(inst.matrix, 12)));
    if (!inst.dynamic) bvh::g_pool.revision += 1u;
    inst.dynamic = true;
    inst.last_follow_frame = frame;
    bvh::g_pool.dynamic_revision += 1u;
    return;
  }
}

static uint64_t RefitStateHash(bvh::BvhDeviceData* data, uint64_t h) {
  HashBytes(&h, data->tlas_instances_cpu.data(), data->tlas_instances_cpu.size() * sizeof(bvh::WorldInstanceGPU));
  HashBytes(&h, data->tlas_leaves_cpu.data(), data->tlas_leaves_cpu.size() * sizeof(bvh::BVHLeafGPU));
  HashBytes(&h, data->tlas_nodes_cpu.data(), data->tlas_nodes_cpu.size() * sizeof(bvh::BVHNodeGPU));
  HashBytes(&h, data->tlas_leaf_of_instance.data(), data->tlas_leaf_of_instance.size() * sizeof(uint32_t));
  for (const bvh::LiveTlasInstance& t : data->tlas_info) {
    HashBytes(&h, &t.id, sizeof t.id);
    HashBytes(&h, &t.uid, sizeof t.uid);
    HashBytes(&h, t.matrix, sizeof t.matrix);
    HashBytes(&h, t.bounds_min, sizeof t.bounds_min);
    HashBytes(&h, t.bounds_max, sizeof t.bounds_max);
    HashBytes(&h, &t.visibility.inputs.valid, sizeof t.visibility.inputs.valid);
    HashBytes(&h, t.visibility.inputs.color, sizeof t.visibility.inputs.color);
    HashBytes(&h, t.visibility.inputs.param, sizeof t.visibility.inputs.param);
  }
  const bvh::LiveBvhStats& s = data->live;
  HashBytes(&h, &s.tlas_refits, sizeof s.tlas_refits);
  HashBytes(&h, &s.refit_instances, sizeof s.refit_instances);
  HashBytes(&h, &s.refit_missing, sizeof s.refit_missing);
  HashBytes(&h, &s.refit_failures, sizeof s.refit_failures);
  HashBytes(&h, &s.tlas_invalid, sizeof s.tlas_invalid);
  HashBytes(&h, &s.tlas_rebuilds, sizeof s.tlas_rebuilds);
  const uint8_t failed = data->tlas_refit_failed ? 1u : 0u;
  HashBytes(&h, &failed, sizeof failed);
  HashBytes(&h, s.last_failure.data(), s.last_failure.size());
  return h;
}

// Copy of the HEAD SortBvhLeaves (Morton codes, then std::sort on (code, prim) keys): the reference for TestSortExact.
static void HeadSortLeaves(std::vector<bvh::BVHLeafGPU>* leaves) {
  if (leaves->empty()) return;
  float bounds_min[3] = {1e30f, 1e30f, 1e30f};
  float bounds_max[3] = {-1e30f, -1e30f, -1e30f};
  for (const bvh::BVHLeafGPU& leaf : *leaves) {
    for (int k = 0; k < 3; ++k) {
      bounds_min[k] = (std::min)(bounds_min[k], leaf.bounds_min[k]);
      bounds_max[k] = (std::max)(bounds_max[k], leaf.bounds_max[k]);
    }
  }
  for (int k = 0; k < 3; ++k) {
    if (!(bounds_max[k] > bounds_min[k])) {
      bounds_min[k] -= 1e-3f;
      bounds_max[k] += 1e-3f;
    }
  }
  const float extent[3] = {
      bounds_max[0] - bounds_min[0],
      bounds_max[1] - bounds_min[1],
      bounds_max[2] - bounds_min[2]};
  for (bvh::BVHLeafGPU& leaf : *leaves) {
    const float cx = (leaf.bounds_min[0] + leaf.bounds_max[0]) * 0.5f;
    const float cy = (leaf.bounds_min[1] + leaf.bounds_max[1]) * 0.5f;
    const float cz = (leaf.bounds_min[2] + leaf.bounds_max[2]) * 0.5f;
    leaf.code = bvh::Morton3D(
        (cx - bounds_min[0]) / extent[0],
        (cy - bounds_min[1]) / extent[1],
        (cz - bounds_min[2]) / extent[2]);
  }
  // Sort 16-byte keys instead of 48-byte leaves, then gather.
  struct Key {
    uint64_t order;
    uint32_t index;
  };
  std::vector<Key> keys(leaves->size());
  for (uint32_t i = 0; i < keys.size(); ++i) {
    keys[i].order = (static_cast<uint64_t>((*leaves)[i].code) << 32u) | (*leaves)[i].prim;
    keys[i].index = i;
  }
  std::sort(keys.begin(), keys.end(), [](const Key& a, const Key& b) { return a.order < b.order; });
  std::vector<bvh::BVHLeafGPU> sorted(leaves->size());
  for (size_t i = 0; i < keys.size(); ++i) sorted[i] = (*leaves)[keys[i].index];
  leaves->swap(sorted);
}

static void TestSortExact() {
  std::mt19937 rng(7u);
  const size_t sizes[5] = {0, 1, 2, 5, 25000};
  int cases = 0;
  for (int variant = 0; variant < 4; ++variant) {
    for (size_t n : sizes) {
      std::vector<bvh::BVHLeafGPU> input(n);
      std::vector<uint32_t> prims(n);
      for (uint32_t i = 0; i < n; ++i) prims[i] = i;
      if (variant == 2) std::shuffle(prims.begin(), prims.end(), rng);  // non-ascending prims: fallback path
      for (size_t i = 0; i < n; ++i) {
        const float c = (variant == 1) ? static_cast<float>(i % 7u) : (variant == 3) ? 0.5f : static_cast<float>(rng() % 1000u) * 0.01f;
        const float lo[3] = {c, c * 0.5f, -c};
        const float hi[3] = {c + 0.3f, c * 0.5f + 0.2f, -c + 0.1f};
        input[i] = bvh::MakeLeaf(lo, hi, prims[i]);
      }
      std::vector<bvh::BVHLeafGPU> head = input;
      std::vector<bvh::BVHLeafGPU> fresh = input;
      HeadSortLeaves(&head);
      bvh::SortBvhLeaves(&fresh);
      CHECK(head.size() == fresh.size() && (n == 0 || std::memcmp(head.data(), fresh.data(), n * sizeof(bvh::BVHLeafGPU)) == 0),
            "sort variant %d size %zu equals HEAD bytewise", variant, n);
      ++cases;
    }
  }
  std::printf("TestSortExact: %d cases\n", cases);
}

static void TestRefitGolden() {
  SkipRig rig;
  bvh::BvhDeviceData* data = rig.data();
  const uint64_t kA = 0xA1u, kB = 0xB2u, kC = 0xC3u, kS = 0xD4u;
  AddMesh(kA, BoxMesh(0.8f));
  AddMesh(kB, Grid(9, 9, 4.f, 0.7f));
  AddMesh(kC, BoxMesh(1.2f));
  AddMesh(kS, BoxMesh(0.5f));
  const uint64_t dyn_keys[3] = {kA, kB, kC};
  for (int i = 0; i < 12; ++i) {
    AddInstance(dyn_keys[i % 3], 2.f * static_cast<float>(i), 0.f, 12.f);
    if (i % 2 == 0) AddInstance(kS, 2.f * static_cast<float>(i) + 1.f, 0.f, 20.f);
  }
  std::vector<uint64_t> dyn;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    for (const bvh::WorldInstance& inst : bvh::g_pool.instances) {
      if (inst.mesh_key != kS) dyn.push_back(inst.id);
    }
  }
  CHECK(dyn.size() == 12u, "12 dynamic candidates (%zu)", dyn.size());
  uint64_t g = 14695981039346656037ull;
  const auto step = [&](int presents) {
    for (int i = 0; i < presents; ++i) {
      rig.present();
      g = RefitStateHash(data, g);
    }
  };
  const auto gpu = [&]() {
    bvh::g_live_bvh.check_requested.store(true);
    rig.present();
    g = RefitStateHash(data, g);
    CHECK(data->live.gpu_ok, "GPU check: %s", data->live.gpu_result.c_str());
  };
  const auto frame = [] { return falcom_world::g_state.frame.load(); };
  step(12);
  for (size_t i = 0; i < dyn.size(); ++i) MoveRefitInstance(dyn[i], 2.f * static_cast<float>(i) + 0.4f, 12.f, frame());
  step(10);
  gpu();
  MoveRefitInstance(dyn[0], 0.9f, 12.f, frame());
  MoveRefitInstance(dyn[3], 6.9f, 12.f, frame());
  MoveRefitInstance(dyn[6], 12.9f, 12.f, frame());
  step(10);
  step(5);
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    for (bvh::WorldInstance& inst : bvh::g_pool.instances) {
      if (inst.id == dyn[1]) inst.bounds_min[0] = std::nanf("");
    }
    bvh::g_pool.dynamic_revision += 1u;
  }
  step(3);
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    for (bvh::WorldInstance& inst : bvh::g_pool.instances) {
      if (inst.id == dyn[4]) inst.bounds_min[1] = inst.bounds_max[1] + 1.f;
    }
    bvh::g_pool.dynamic_revision += 1u;
  }
  step(3);
  MoveRefitInstance(dyn[1], 2.f + 0.4f, 12.f, frame());
  MoveRefitInstance(dyn[4], 8.f + 0.4f, 12.f, frame());
  step(5);
  gpu();
  RetireMesh(kB);
  step(5);
  gpu();
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    for (auto it = bvh::g_pool.instances.begin(); it != bvh::g_pool.instances.end(); ++it) {
      if (it->id != dyn[0]) continue;
      bvh::g_pool.admitted_keys.erase(bvh::PoolAdmittedKey(it->mesh_id, falcom_world::MatrixHash(it->matrix, 12)));
      const auto observed = bvh::g_pool.observations.find(bvh::InstanceKey{kA, falcom_world::MatrixHash(it->matrix, 12)});
      if (observed != bvh::g_pool.observations.end()) observed->second.admitted = false;
      bvh::g_pool.instances.erase(it);
      break;
    }
    bvh::g_pool.dynamic_revision += 1u;
  }
  step(3);
  bvh::g_pool.alpha_wind_sway.store(false);
  step(3);
  bvh::g_pool.alpha_wind_sway.store(true);
  step(5);
  gpu();
  CHECK(g == 0x2bf515dfb5956c83ull, "refit golden equals HEAD pin (%016llx)", (unsigned long long)g);
}

// Cursor path (RefitLiveTlas galloping search): the dynamic suffix is ascending by id but interleaved with static
// instances in the pool.
static void TestRefitCursorInterleaved() {
  SkipRig rig;
  bvh::BvhDeviceData* data = rig.data();
  const uint64_t kA = 0xE1u, kS = 0xE2u;
  AddMesh(kA, BoxMesh(0.8f));
  AddMesh(kS, BoxMesh(0.5f));
  for (int i = 0; i < 8; ++i) {
    AddInstance(kA, 2.f * static_cast<float>(i), 0.f, 12.f);
    AddInstance(kS, 2.f * static_cast<float>(i) + 1.f, 0.f, 20.f);
  }
  std::vector<uint64_t> dyn;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    for (const bvh::WorldInstance& inst : bvh::g_pool.instances) {
      if (inst.mesh_key == kA) dyn.push_back(inst.id);
    }
  }
  uint64_t g = 14695981039346656037ull;
  const auto step = [&](int presents) {
    for (int i = 0; i < presents; ++i) {
      rig.present();
      g = RefitStateHash(data, g);
    }
  };
  const auto frame = [] { return falcom_world::g_state.frame.load(); };
  step(12);
  for (size_t i = 0; i < dyn.size(); ++i) MoveRefitInstance(dyn[i], 2.f * static_cast<float>(i) + 0.4f, 12.f, frame());
  step(10);
  MoveRefitInstance(dyn[2], 4.9f, 12.f, frame());
  MoveRefitInstance(dyn[6], 12.9f, 12.f, frame());
  step(5);
  std::printf("refit cursor interleaved: missing %u, instances %u\n", data->live.refit_missing, data->live.refit_instances);
  CHECK(g == 0xcd7b1032cf8bae98ull, "refit cursor interleaved pin (%016llx)", (unsigned long long)g);
}

// Cursor miss (fallback path): dynamic instances removed from the pool in the middle of the suffix. The galloping
// search cannot find them, so the full lower_bound decides and each one is counted as missing.
static void TestRefitCursorMiss() {
  SkipRig rig;
  bvh::BvhDeviceData* data = rig.data();
  const uint64_t kA = 0xE3u, kS = 0xE4u;
  AddMesh(kA, BoxMesh(0.8f));
  AddMesh(kS, BoxMesh(0.5f));
  for (int i = 0; i < 8; ++i) {
    AddInstance(kA, 2.f * static_cast<float>(i), 0.f, 12.f);
    AddInstance(kS, 2.f * static_cast<float>(i) + 1.f, 0.f, 20.f);
  }
  std::vector<uint64_t> dyn;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    for (const bvh::WorldInstance& inst : bvh::g_pool.instances) {
      if (inst.mesh_key == kA) dyn.push_back(inst.id);
    }
  }
  uint64_t g = 14695981039346656037ull;
  const auto step = [&](int presents) {
    for (int i = 0; i < presents; ++i) {
      rig.present();
      g = RefitStateHash(data, g);
    }
  };
  const auto frame = [] { return falcom_world::g_state.frame.load(); };
  step(12);
  for (size_t i = 0; i < dyn.size(); ++i) MoveRefitInstance(dyn[i], 2.f * static_cast<float>(i) + 0.4f, 12.f, frame());
  step(10);
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    for (const uint64_t id : {dyn[2], dyn[5]}) {
      for (auto it = bvh::g_pool.instances.begin(); it != bvh::g_pool.instances.end(); ++it) {
        if (it->id != id) continue;
        bvh::g_pool.admitted_keys.erase(bvh::PoolAdmittedKey(it->mesh_id, falcom_world::MatrixHash(it->matrix, 12)));
        bvh::g_pool.instances.erase(it);
        break;
      }
    }
    bvh::g_pool.dynamic_revision += 1u;
  }
  step(5);
  std::printf("refit cursor miss: missing %u, instances %u\n", data->live.refit_missing, data->live.refit_instances);
  CHECK(g == 0x969d56d0bdf3e0c6ull, "refit cursor miss pin (%016llx)", (unsigned long long)g);
}

static falcom_world::WindSwayParams RestWindSway(int p) {
  falcom_world::WindSwayParams wind;
  wind.wave_frequency = 1.f;
  wind.wind_force = 2.f + 0.01f * static_cast<float>(p);
  wind.wind_dir[0] = 1.f;
  wind.shake_scale = 0.5f;
  wind.shake_speed = 1.f;
  wind.shake_freq = 1.f;
  wind.shake_wind_scale = 1.f;
  wind.shake_flexibility = 3.f;
  wind.layout = "test";
  wind.wave_time = 0.1f * static_cast<float>(p);
  wind.game_time = 0.1f * static_cast<float>(p);
  return wind;
}

// Golden for the wind sway refit at the default range: eight wind instances at 15 to 200 m (Chebyshev from the camera
// at (10,5,10)) are swayed every present, and a near and a far non-wind dynamic instance follow their poses. At the
// default (maximum) range nothing is gated, so this pin holds across the wind range change.
static void TestWindRangeGolden() {
  SkipRig rig;
  bvh::BvhDeviceData* data = rig.data();
  const uint64_t kW = 0xF1u, kD = 0xF2u;
  AddMesh(kW, BoxMesh(1.3f));
  AddMesh(kD, BoxMesh(0.8f));
  const float wind_d[8] = {15.f, 15.f, 40.f, 40.f, 70.f, 70.f, 120.f, 200.f};
  for (int i = 0; i < 8; ++i) AddInstance(kW, 10.f + wind_d[i], 0.f, 10.f + 0.5f * static_cast<float>(i));
  AddInstance(kD, 14.f, 0.f, 12.f);
  AddInstance(kD, 210.f, 0.f, 10.f);
  std::vector<uint64_t> dyn;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    for (const bvh::WorldInstance& inst : bvh::g_pool.instances) {
      if (inst.mesh_key == kD) dyn.push_back(inst.id);
    }
    bvh::g_pool.wind_keys.insert(kW);
    bvh::g_pool.wind_sway[kW] = RestWindSway(0);
  }
  CHECK(dyn.size() == 2u, "two non-wind dynamic candidates (%zu)", dyn.size());
  uint64_t g = 14695981039346656037ull;
  const auto frame = [] { return falcom_world::g_state.frame.load(); };
  for (int i = 0; i < 40; ++i) {
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      bvh::g_pool.wind_sway[kW].wave_time = 0.1f * static_cast<float>(i);
      bvh::g_pool.wind_sway[kW].game_time = 0.1f * static_cast<float>(i);
    }
    MoveRefitInstance(dyn[0], 14.f + 0.05f * static_cast<float>(i), 12.f, frame());
    MoveRefitInstance(dyn[1], 210.f, 10.f + 0.05f * static_cast<float>(i), frame());
    rig.present();
    g = RefitStateHash(data, g);
  }
  CHECK(data->tlas_wind_count == 8u, "eight wind instances in the TLAS (%u)", data->tlas_wind_count);
  std::printf("wind range golden %016llx\n", (unsigned long long)g);
  CHECK(g == 0x1fd318de2089b679ull, "wind range golden equals HEAD pin (%016llx)", (unsigned long long)g);
}

// Wind range gate (refit only). Fresh rig per run: eight wind instances at kWindRangeDistance (Chebyshev from the camera
// at (10,5,10)) and two non-wind dynamic instances (one near, one at 210 m). range_at[p] is the wind sway range set before
// present p. The TLAS pose of every instance is recorded after each present.
using WindPoses = std::vector<std::array<float, 16>>;  // the 8 wind instances, then the 2 non-wind dynamic ones
struct WindRangeRun {
  std::vector<WindPoses> poses;
  std::vector<uint32_t> far_skipped;
  std::vector<uint64_t> wind_ids;  // ascending id: wind_ids[i] is at kWindRangeDistance[i]
  std::vector<uint64_t> dyn_ids;   // [0] near, [1] far
  bool gpu_ok = false;
};
static const float kWindRangeDistance[8] = {15.f, 15.f, 40.f, 40.f, 70.f, 70.f, 120.f, 200.f};

static WindRangeRun RunWindRange(const std::vector<float>& range_at) {
  SkipRig rig;
  bvh::BvhDeviceData* data = rig.data();
  const uint64_t kW = 0xF3u, kD = 0xF4u;
  AddMesh(kW, BoxMesh(1.3f));
  AddMesh(kD, BoxMesh(0.8f));
  for (int i = 0; i < 8; ++i) AddInstance(kW, 10.f + kWindRangeDistance[i], 0.f, 10.f + 0.5f * static_cast<float>(i));
  AddInstance(kD, 14.f, 0.f, 12.f);
  AddInstance(kD, 210.f, 0.f, 10.f);
  WindRangeRun run;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    for (const bvh::WorldInstance& inst : bvh::g_pool.instances) {
      if (inst.mesh_key == kW) run.wind_ids.push_back(inst.id);
      if (inst.mesh_key == kD) run.dyn_ids.push_back(inst.id);
    }
    bvh::g_pool.wind_keys.insert(kW);
    bvh::g_pool.wind_sway[kW] = RestWindSway(0);
  }
  const auto frame = [] { return falcom_world::g_state.frame.load(); };
  const auto record = [&]() {
    std::map<uint64_t, std::array<float, 16>> by_id;  // ids are global across runs: compare by index
    for (const bvh::LiveTlasInstance& t : data->tlas_info) std::memcpy(by_id[t.id].data(), t.matrix, sizeof(t.matrix));
    WindPoses poses;
    for (const uint64_t id : run.wind_ids) poses.push_back(by_id[id]);
    for (const uint64_t id : run.dyn_ids) poses.push_back(by_id[id]);
    run.poses.push_back(poses);
    run.far_skipped.push_back(bvh::g_pool.stats.wind_pass.far_skipped);
  };
  for (size_t p = 0; p < range_at.size(); ++p) {
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      const float wave = 0.1f * static_cast<float>(p);
      bvh::g_pool.wind_sway[kW].wave_time = wave;
      bvh::g_pool.wind_sway[kW].game_time = wave;
      bvh::g_pool.wind_sway_range = range_at[p];
    }
    MoveRefitInstance(run.dyn_ids[0], 14.f + 0.05f * static_cast<float>(p), 12.f, frame());
    MoveRefitInstance(run.dyn_ids[1], 210.f, 10.f + 0.05f * static_cast<float>(p), frame());
    rig.present();
    record();
  }
  bvh::g_live_bvh.check_requested.store(true);
  rig.present();
  run.gpu_ok = data->live.gpu_ok;
  return run;
}

static void TestWindRangeGate() {
  const int kPresents = 30;
  const std::vector<float> all_max(kPresents, 1044.f);
  const WindRangeRun full = RunWindRange(all_max);
  std::vector<float> near_only(kPresents, 50.f);
  const WindRangeRun gated = RunWindRange(near_only);
  std::vector<float> raised(kPresents, 50.f);
  for (int p = 15; p < kPresents; ++p) raised[p] = 1044.f;
  const WindRangeRun resumed = RunWindRange(raised);
  const WindRangeRun wide(RunWindRange(std::vector<float>(kPresents, 1043.f)));

  // Part 2: inside 50 m the refit matches the max-range run every present; beyond it the pose is frozen.
  size_t near_mismatch = 0, far_moving = 0, follow_mismatch = 0;
  size_t first_gated = kPresents;
  for (size_t p = 0; p < gated.poses.size(); ++p) {
    if (gated.far_skipped[p] != 0u && first_gated == kPresents) first_gated = p;
  }
  CHECK(first_gated > 0u && first_gated < static_cast<size_t>(kPresents), "gate engaged on a refit present (%zu)", first_gated);
  const size_t frozen = first_gated - 1u;  // the pose the far instances keep: the TLAS before the first gated refit
  for (size_t p = 0; p < gated.poses.size(); ++p) {
    for (size_t i = 0; i < 4; ++i) {
      if (gated.poses[p][i] != full.poses[p][i]) ++near_mismatch;
    }
    for (size_t i = 4; i < 8; ++i) {
      if (p >= first_gated && gated.poses[p][i] != gated.poses[frozen][i]) ++far_moving;
    }
    for (size_t d = 0; d < 2; ++d) {
      if (gated.poses[p][8 + d] != full.poses[p][8 + d]) ++follow_mismatch;
    }
  }
  CHECK(near_mismatch == 0u, "range 50: instances within 50 m match the max-range run every present (%zu mismatches)", near_mismatch);
  CHECK(far_moving == 0u, "range 50: instances beyond 50 m stay at their first gated pose (%zu changes)", far_moving);
  CHECK(follow_mismatch == 0u, "range 50: the non-wind dynamic instances follow as at max (%zu mismatches)", follow_mismatch);
  CHECK(full.poses[kPresents - 1][7] != full.poses[frozen][7], "max range: the far instance moves (sanity)");
  bool far_count_ok = true;
  for (size_t p = first_gated; p < gated.far_skipped.size(); ++p) far_count_ok = far_count_ok && gated.far_skipped[p] == 4u;
  CHECK(far_count_ok, "range 50: far_skipped equals the 4 instances beyond 50 m every refit present");
  CHECK(gated.gpu_ok && full.gpu_ok, "GPU check passes at range 50 and at max");

  // Part 3: raised back to max at present 15: everything matches the max-range run from then on.
  size_t resumed_mismatch = 0;
  for (size_t p = 15; p < resumed.poses.size(); ++p) {
    if (resumed.poses[p] != full.poses[p]) ++resumed_mismatch;
  }
  CHECK(resumed_mismatch == 0u, "range raised to max: far instances resume and match the max-range run (%zu presents differ)", resumed_mismatch);
  CHECK(resumed.far_skipped[15] == 0u, "range raised to max: no skip at the raise (%u)", resumed.far_skipped[15]);

  // Part 4: 1043 m: the gate runs, nothing is beyond it.
  bool wide_same = wide.poses.size() == full.poses.size();
  for (size_t p = 0; wide_same && p < wide.poses.size(); ++p) wide_same = wide.poses[p] == full.poses[p] && wide.far_skipped[p] == 0u;
  CHECK(wide_same, "range 1043: poses equal the max-range run and far_skipped is 0 every present");
}

// Rest-pose wind refit. Pool: 12 rest wind instances on two wind keys (15-200 m from the camera at (10,5,10)), 3 followed
// wind instances, 3 non-wind dynamic, 6 static. The sway params change every present; the sway switch is off for presents
// 12-17, one key is erased for presents 14-21, one wave_time is NaN at present 20, the range is 50 from present 30.
// force_dynamic marks the rest instances dynamic in the pool (the live path): the TLAS bytes must not depend on it.
struct RestWindRun {
  uint64_t golden = 14695981039346656037ull;
  std::vector<std::vector<uint8_t>> tlas;  // per present: poses, inverses, bounds, descriptors
  size_t rest_mismatch = 0u;               // rest entries that disagree with their pool instance
  bool gpu_ok = false;
};

static const bvh::WorldInstance* PoolInstance(uint64_t id) {  // caller holds g_pool.mutex
  for (const bvh::WorldInstance& inst : bvh::g_pool.instances) {
    if (inst.id == id) return &inst;
  }
  return nullptr;
}

static void FlipCameraSighting(uint64_t id) {  // caller holds g_pool.mutex: the instance's camera view changes
  const bvh::WorldInstance* inst = PoolInstance(id);
  const auto observed = bvh::g_pool.observations.find(bvh::InstanceKey{inst->mesh_key, falcom_world::MatrixHash(inst->matrix, 12)});
  observed->second.camera_sightings = observed->second.camera_sightings == 0u ? 1u : 0u;
  bvh::NotePoolVisibilityChange();
}

static RestWindRun RunRestWind(bool force_dynamic) {
  SkipRig rig;
  bvh::BvhDeviceData* data = rig.data();
  const uint64_t kW1 = 0xE1u, kW2 = 0xE2u, kD = 0xE3u, kS = 0xE4u;
  AddMesh(kW1, BoxMesh(1.3f));
  AddMesh(kW2, Grid(5, 5, 2.f, 0.4f));
  AddMesh(kD, BoxMesh(0.8f));
  AddMesh(kS, BoxMesh(0.5f));
  const float rest_d[12] = {15.f, 20.f, 30.f, 45.f, 60.f, 75.f, 90.f, 110.f, 130.f, 150.f, 175.f, 200.f};
  for (int i = 0; i < 12; ++i) AddInstance(i % 2 == 0 ? kW1 : kW2, 10.f + rest_d[i], 0.f, 10.f + 0.5f * static_cast<float>(i));
  for (int i = 0; i < 3; ++i) AddInstance(kW1, 12.f + static_cast<float>(i), 0.f, 25.f + static_cast<float>(i));
  for (int i = 0; i < 3; ++i) AddInstance(kD, 14.f + static_cast<float>(i), 0.f, 12.f);
  for (int i = 0; i < 6; ++i) AddInstance(kS, 2.f * static_cast<float>(i), 0.f, 40.f);
  std::vector<uint64_t> rest, followed, dyn;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    std::vector<uint64_t> wind;  // ascending id: the 12 rest instances, then the 3 followed ones
    for (const bvh::WorldInstance& inst : bvh::g_pool.instances) {
      if (inst.mesh_key == kW1 || inst.mesh_key == kW2) wind.push_back(inst.id);
      if (inst.mesh_key == kD) dyn.push_back(inst.id);
    }
    rest.assign(wind.begin(), wind.begin() + 12);
    followed.assign(wind.begin() + 12, wind.end());
    bvh::g_pool.wind_keys.insert(kW1);
    bvh::g_pool.wind_keys.insert(kW2);
  }
  CHECK(rest.size() == 12u && followed.size() == 3u && dyn.size() == 3u, "pool split %zu/%zu/%zu", rest.size(), followed.size(), dyn.size());
  RestWindRun run;
  const auto frame = [] { return falcom_world::g_state.frame.load(); };
  const int kPresents = 40;
  for (int p = 0; p < kPresents; ++p) {
    const bool kw2_out = p >= 14 && p < 22;
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      const bool sway_on = p < 12 || p >= 18;
      bvh::g_pool.alpha_wind_sway.store(sway_on);
      bvh::g_pool.wind_sway[kW1] = RestWindSway(p);
      if (kw2_out) {
        bvh::g_pool.wind_sway.erase(kW2);
      } else {
        bvh::g_pool.wind_sway[kW2] = RestWindSway(p);
      }
      // Forced rest instances (key kW1, swayed while the switch is on) are pool-dynamic: the live path. Their TLAS
      // partition is then the same as the rest path's, so the bytes compare.
      for (bvh::WorldInstance& inst : bvh::g_pool.instances) {
        if (inst.mesh_key == kW1 && std::find(rest.begin(), rest.end(), inst.id) != rest.end()) inst.dynamic = force_dynamic && sway_on;
      }
      if (p == 20) bvh::g_pool.wind_sway[kW1].wave_time = std::nanf("");
      if (p == 30) bvh::g_pool.wind_sway_range = 50.f;
      if (p == 10) FlipCameraSighting(rest[0]);
      if (p == 25) FlipCameraSighting(rest[1]);
    }
    for (size_t i = 0; i < followed.size(); ++i) {
      MoveRefitInstance(followed[i], 12.f + static_cast<float>(i) + 0.02f * static_cast<float>(p), 25.f + static_cast<float>(i), frame());
    }
    for (size_t i = 0; i < dyn.size(); ++i) MoveRefitInstance(dyn[i], 14.f + static_cast<float>(i) + 0.05f * static_cast<float>(p), 12.f, frame());
    rig.present();
    run.golden = RefitStateHash(data, run.golden);
    const bvh::PoolWindPass& pass = bvh::g_pool.stats.wind_pass;
    HashBytes(&run.golden, &pass.applied, sizeof pass.applied);
    HashBytes(&run.golden, &pass.zero_disp, sizeof pass.zero_disp);
    HashBytes(&run.golden, &pass.clamped, sizeof pass.clamped);
    HashBytes(&run.golden, &pass.skipped_nonfinite, sizeof pass.skipped_nonfinite);
    HashBytes(&run.golden, &pass.skipped_range, sizeof pass.skipped_range);
    HashBytes(&run.golden, &pass.no_inputs, sizeof pass.no_inputs);
    HashBytes(&run.golden, &pass.far_skipped, sizeof pass.far_skipped);
    HashBytes(&run.golden, &bvh::g_pool.stats.wind_sway_instances, sizeof bvh::g_pool.stats.wind_sway_instances);
    std::vector<uint8_t>& bytes = run.tlas.emplace_back();
    const auto append = [&bytes](const void* p, size_t n) {
      const uint8_t* b = static_cast<const uint8_t*>(p);
      bytes.insert(bytes.end(), b, b + n);
    };
    for (const bvh::LiveTlasInstance& t : data->tlas_info) {
      append(t.matrix, sizeof t.matrix);
      append(t.inverse_world, sizeof t.inverse_world);
      append(t.bounds_min, sizeof t.bounds_min);
      append(t.bounds_max, sizeof t.bounds_max);
    }
    append(data->tlas_instances_cpu.data(), data->tlas_instances_cpu.size() * sizeof(bvh::WorldInstanceGPU));
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      for (const bvh::LiveTlasInstance& t : data->tlas_info) {
        if (std::find(rest.begin(), rest.end(), t.id) == rest.end()) continue;
        const bvh::WorldInstance* inst = PoolInstance(t.id);
        if (inst == nullptr) {
          run.rest_mismatch += 1u;
        } else if (t.wind) {  // swayed: the sway only widens the bounds around the rest pose
          for (int k = 0; k < 3; ++k) {
            if (t.bounds_min[k] > inst->bounds_min[k] || t.bounds_max[k] < inst->bounds_max[k]) run.rest_mismatch += 1u;
          }
        } else if (std::memcmp(t.matrix, inst->matrix, sizeof t.matrix) != 0 || std::memcmp(t.inverse_world, inst->inverse_world, sizeof t.inverse_world) != 0
                   || std::memcmp(t.bounds_min, inst->bounds_min, sizeof t.bounds_min) != 0 || std::memcmp(t.bounds_max, inst->bounds_max, sizeof t.bounds_max) != 0) {
          run.rest_mismatch += 1u;
        }
      }
    }
  }
  bvh::g_live_bvh.check_requested.store(true);
  rig.present();
  run.golden = RefitStateHash(data, run.golden);
  run.gpu_ok = data->live.gpu_ok;
  return run;
}

static void TestRestWindGolden() {
  const RestWindRun run = RunRestWind(false);
  std::printf("rest wind golden %016llx\n", (unsigned long long)run.golden);
  CHECK(run.rest_mismatch == 0u, "rest entries equal their pool instances (sway-widened bounds aside): %zu mismatches", run.rest_mismatch);
  CHECK(run.gpu_ok, "GPU check passes after the rest wind run");
  CHECK(run.golden == 0x427521e3761f0d08ull, "rest wind golden equals HEAD pin (%016llx)", (unsigned long long)run.golden);
}

static void TestRestWindFastSlowEqual() {
  const RestWindRun fast = RunRestWind(false);
  const RestWindRun slow = RunRestWind(true);
  CHECK(fast.tlas.size() == 40u && slow.tlas.size() == fast.tlas.size(), "same present count (%zu, %zu)", fast.tlas.size(), slow.tlas.size());
  size_t differ = 0u;
  for (size_t p = 0; p < fast.tlas.size() && p < slow.tlas.size(); ++p) differ += fast.tlas[p] != slow.tlas[p] ? 1u : 0u;
  CHECK(differ == 0u, "rest instances forced pool-dynamic: TLAS poses, inverses, bounds and descriptors equal every present (%zu differ)", differ);
  CHECK(slow.rest_mismatch == 0u && slow.gpu_ok, "forced run: rest entries match the pool and the GPU check passes");
}

// Followed wind instance crossing the range both ways: at range 50 it is moved beyond the range for presents 12-19 and back
// inside from present 20. From the return on, its pose equals the max-range run every present.
static std::vector<std::array<float, 16>> RunFollowedCrossing(float range, std::vector<uint32_t>* far_skipped) {
  SkipRig rig;
  bvh::BvhDeviceData* data = rig.data();
  const uint64_t kW = 0xE9u;
  AddMesh(kW, BoxMesh(1.3f));
  AddInstance(kW, 14.f, 0.f, 12.f);
  uint64_t id = 0u;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    id = bvh::g_pool.instances.back().id;
    bvh::g_pool.wind_keys.insert(kW);
    bvh::g_pool.wind_sway[kW] = RestWindSway(0);
    bvh::g_pool.wind_sway_range = range;
  }
  const auto frame = [] { return falcom_world::g_state.frame.load(); };
  std::vector<std::array<float, 16>> poses;
  for (int p = 0; p < 30; ++p) {
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      bvh::g_pool.wind_sway[kW] = RestWindSway(p);
    }
    const float x = p >= 12 && p < 20 ? 210.f : 14.f + 0.02f * static_cast<float>(p);
    MoveRefitInstance(id, x, 12.f, frame());
    rig.present();
    std::array<float, 16> pose = {};
    for (const bvh::LiveTlasInstance& t : data->tlas_info) {
      if (t.id == id) std::memcpy(pose.data(), t.matrix, sizeof(t.matrix));
    }
    poses.push_back(pose);
    far_skipped->push_back(bvh::g_pool.stats.wind_pass.far_skipped);
  }
  return poses;
}

static void TestFollowedWindCrossing() {
  std::vector<uint32_t> gated_far, full_far;
  const auto gated = RunFollowedCrossing(50.f, &gated_far);
  const auto full = RunFollowedCrossing(1044.f, &full_far);
  size_t resume_mismatch = 0u;
  for (size_t p = 20; p < gated.size(); ++p) resume_mismatch += gated[p] != full[p] ? 1u : 0u;
  CHECK(gated_far[15] == 1u, "far leg: the followed instance is skipped beyond 50 m (%u)", gated_far[15]);
  CHECK(resume_mismatch == 0u, "followed instance back inside 50 m resumes: its pose matches the max-range run (%zu presents differ)", resume_mismatch);
}

// Lag counts followed instances only: a scene of rest wind instances alone has no lag.
static void TestRestWindLag() {
  SkipRig rig;
  bvh::BvhDeviceData* data = rig.data();
  const uint64_t kW = 0xEAu;
  AddMesh(kW, BoxMesh(1.3f));
  for (int i = 0; i < 4; ++i) AddInstance(kW, 12.f + static_cast<float>(i), 0.f, 12.f);
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    bvh::g_pool.wind_keys.insert(kW);
    bvh::g_pool.wind_sway[kW] = RestWindSway(0);
  }
  for (int p = 0; p < 20; ++p) {
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      bvh::g_pool.wind_sway[kW] = RestWindSway(p);
      bvh::g_pool.dynamic_revision += 1u;
    }
    rig.present();
  }
  CHECK(data->live.tlas_refits > 0u, "refits ran (%u)", data->live.tlas_refits);
  CHECK(data->live.refit_lag_max == 0u, "rest wind only: refit lag stays 0 (%u)", data->live.refit_lag_max);
}

// A far rest instance removed from the pool leaves the TLAS by the next rebuild, with no missing count or failure. A far
// followed instance removed from the pool is counted missing.
static void TestRemoveFarWind() {
  SkipRig rig;
  bvh::BvhDeviceData* data = rig.data();
  const uint64_t kW = 0xEBu, kD = 0xECu;
  AddMesh(kW, BoxMesh(1.3f));
  AddMesh(kD, BoxMesh(0.8f));
  AddInstance(kW, 210.f, 0.f, 10.f);  // rest, far
  AddInstance(kW, 212.f, 0.f, 10.f);  // followed, far
  AddInstance(kD, 14.f, 0.f, 12.f);   // non-wind dynamic, near (keeps the refit running)
  uint64_t rest_id = 0u, followed_id = 0u, dyn_id = 0u;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    rest_id = bvh::g_pool.instances[bvh::g_pool.instances.size() - 3u].id;
    followed_id = bvh::g_pool.instances[bvh::g_pool.instances.size() - 2u].id;
    dyn_id = bvh::g_pool.instances.back().id;
    bvh::g_pool.wind_keys.insert(kW);
    bvh::g_pool.wind_sway[kW] = RestWindSway(0);
    bvh::g_pool.wind_sway_range = 50.f;
  }
  const auto frame = [] { return falcom_world::g_state.frame.load(); };
  const auto erase = [](uint64_t id) {
    std::lock_guard lock(bvh::g_pool.mutex);
    for (auto it = bvh::g_pool.instances.begin(); it != bvh::g_pool.instances.end(); ++it) {
      if (it->id != id) continue;
      bvh::g_pool.admitted_keys.erase(bvh::PoolAdmittedKey(it->mesh_id, falcom_world::MatrixHash(it->matrix, 12)));
      bvh::g_pool.instances.erase(it);
      break;
    }
    bvh::BumpPoolRevision(bvh::PoolRevisionSource::Retire);
  };
  for (int p = 0; p < 3; ++p) {
    MoveRefitInstance(followed_id, 212.f + 0.02f * static_cast<float>(p), 10.f, frame());
    MoveRefitInstance(dyn_id, 14.f + 0.05f * static_cast<float>(p), 12.f, frame());
    rig.present();
  }
  const uint32_t missing_before = data->live.refit_missing, failures_before = data->live.refit_failures;
  erase(rest_id);
  for (uint32_t p = 0; p < bvh::kLiveTlasInterval; ++p) {
    MoveRefitInstance(dyn_id, 14.f + 0.05f * static_cast<float>(p), 12.f, frame());
    rig.present();
  }
  bool rest_in_tlas = false;
  for (const bvh::LiveTlasInstance& t : data->tlas_info) rest_in_tlas = rest_in_tlas || t.id == rest_id;
  CHECK(!rest_in_tlas, "far rest instance removed: gone from the TLAS within kLiveTlasInterval presents");
  CHECK(data->live.refit_missing == missing_before && data->live.refit_failures == failures_before,
        "far rest instance removed: no missing count or refit failure (%u, %u)", data->live.refit_missing - missing_before,
        data->live.refit_failures - failures_before);
  erase(followed_id);
  MoveRefitInstance(dyn_id, 14.f, 12.f, frame());
  rig.present();
  CHECK(data->live.refit_missing == missing_before + 1u, "far followed instance removed: counted missing (%u)", data->live.refit_missing - missing_before);
}

// The GPU check compares TLAS leaves by prim only: stale leaf bounds after a refit pass, a wrong prim fails.
static void TestRefitGpuLeaves() {
  SkipRig rig;
  bvh::BvhDeviceData* data = rig.data();
  const uint64_t kA = 0xEDu;
  AddMesh(kA, BoxMesh(0.8f));
  for (int i = 0; i < 4; ++i) AddInstance(kA, 2.f * static_cast<float>(i), 0.f, 12.f);
  std::vector<uint64_t> dyn;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    for (const bvh::WorldInstance& inst : bvh::g_pool.instances) dyn.push_back(inst.id);
  }
  const auto frame = [] { return falcom_world::g_state.frame.load(); };
  for (int p = 0; p < 4; ++p) {
    for (size_t i = 0; i < dyn.size(); ++i) MoveRefitInstance(dyn[i], 2.f * static_cast<float>(i) + 0.3f * static_cast<float>(p), 12.f, frame());
    rig.present();
  }
  const auto gpu = [&]() {
    bvh::g_live_bvh.check_requested.store(true);
    rig.present();
    return data->live.gpu_ok;
  };
  CHECK(gpu(), "GPU check after refits with stale leaf bounds: %s", data->live.gpu_result.c_str());
  Res& leaves = rig.dev.res[data->tlas_leaf_buffer.handle];
  bvh::BVHLeafGPU leaf;
  std::memcpy(&leaf, leaves.bytes.data(), sizeof leaf);
  bvh::BVHLeafGPU corrupted = leaf;
  corrupted.bounds_max[0] += 50.f;
  std::memcpy(leaves.bytes.data(), &corrupted, sizeof corrupted);
  CHECK(gpu(), "GPU check ignores leaf bounds: %s", data->live.gpu_result.c_str());
  corrupted.prim = leaf.prim + 1u;
  std::memcpy(leaves.bytes.data(), &corrupted, sizeof corrupted);
  CHECK(!gpu(), "GPU check fails a leaf with a wrong prim");
  std::memcpy(leaves.bytes.data(), &leaf, sizeof leaf);
}

// A rest wind instance promoted to followed (it moves in the pool): the refit that follows reads its live pose (the TLAS
// bounds contain its new pool bounds), and the TLAS is rebuilt later without a skip at the promotion. With `slack` the
// static block has holes (the slack apply is tried first).
static bool TlasContainsPool(bvh::BvhDeviceData* data, uint64_t id) {  // caller does not hold the pool lock
  std::lock_guard lock(bvh::g_pool.mutex);
  const bvh::WorldInstance* inst = PoolInstance(id);
  for (const bvh::LiveTlasInstance& t : data->tlas_info) {
    if (t.id != id || inst == nullptr) continue;
    for (int k = 0; k < 3; ++k) {
      if (t.bounds_min[k] > inst->bounds_min[k] || t.bounds_max[k] < inst->bounds_max[k]) return false;
    }
    return true;
  }
  return false;
}

static bool TlasEntryRest(bvh::BvhDeviceData* data, uint64_t id) {
  for (const bvh::LiveTlasInstance& t : data->tlas_info) {
    if (t.id == id) return t.rest;
  }
  return false;
}

// Steady state: with no pool or visibility change, the refit reads no pool entry for its rest wind instances.
static void TestRestSteadyReads() {
  SkipRig rig;
  bvh::BvhDeviceData* data = rig.data();
  const uint64_t kW = 0xF6u, kD = 0xF7u;
  AddMesh(kW, BoxMesh(1.3f));
  AddMesh(kD, BoxMesh(0.8f));
  for (int i = 0; i < 3; ++i) AddInstance(kW, 20.f + 3.f * static_cast<float>(i), 0.f, 12.f);
  AddInstance(kD, 14.f, 0.f, 12.f);
  uint64_t dyn_id = 0u;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    for (const bvh::WorldInstance& inst : bvh::g_pool.instances) {
      if (inst.mesh_key == kD) dyn_id = inst.id;
    }
    bvh::g_pool.wind_keys.insert(kW);
  }
  const auto frame = [] { return falcom_world::g_state.frame.load(); };
  for (int p = 0; p < 3; ++p) {
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      bvh::g_pool.wind_sway[kW] = RestWindSway(p);
    }
    MoveRefitInstance(dyn_id, 14.f + 0.05f * static_cast<float>(p), 12.f, frame());
    rig.present();
  }
  for (int p = 3; p < 8; ++p) {
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      bvh::g_pool.wind_sway[kW] = RestWindSway(p);
      bvh::g_pool.dynamic_revision += 1u;
    }
    rig.present();
    CHECK(data->live.refit_rest_slow == 0u, "steady present %d: no pool read for the rest entries (%u)", p, data->live.refit_rest_slow);
  }
  CHECK(data->live.refit_rest_fast == 3u, "steady state: the 3 rest entries come from rest_pose (%u)", data->live.refit_rest_fast);
}

static void RunRestPromotion(bool slack, const char* label) {
  SkipRig rig;
  bvh::BvhDeviceData* data = rig.data();
  const uint64_t kW = 0xEEu, kD = 0xEFu, kS = 0xF5u;
  AddMesh(kW, BoxMesh(1.3f));
  AddMesh(kD, BoxMesh(0.8f));
  AddMesh(kS, BoxMesh(0.5f));
  AddInstance(kW, 25.f, 0.f, 10.f);
  AddInstance(kD, 14.f, 0.f, 12.f);
  if (slack) {
    for (int i = 0; i < 1100; ++i) AddInstance(kS, 100.f + 2.f * static_cast<float>(i % 50), 0.f, 100.f + 2.f * static_cast<float>(i / 50));
  }
  uint64_t rest_id = 0u, dyn_id = 0u;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    for (const bvh::WorldInstance& inst : bvh::g_pool.instances) {
      if (inst.mesh_key == kW) rest_id = inst.id;
      if (inst.mesh_key == kD) dyn_id = inst.id;
    }
    bvh::g_pool.wind_keys.insert(kW);
  }
  bvh::g_live_bvh.slack.store(slack);
  const auto frame = [] { return falcom_world::g_state.frame.load(); };
  for (int p = 0; p < 4; ++p) {
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      bvh::g_pool.wind_sway[kW] = RestWindSway(p);
    }
    MoveRefitInstance(dyn_id, 14.f + 0.05f * static_cast<float>(p), 12.f, frame());
    rig.present();
  }
  CHECK(data->tlas_wind_count == 1u, "%s: the rest wind instance is in the TLAS (%u)", label, data->tlas_wind_count);
  CHECK(TlasEntryRest(data, rest_id), "%s: the wind instance is a rest entry before its promotion", label);
  const uint32_t skips = data->live.tlas_skips;
  const uint32_t attempts = data->live.tlas_rebuilds + data->live.tlas_skips;  // a skip or a full build (slack applies not counted)
  MoveRefitInstance(rest_id, 30.f, 10.f, frame());
  rig.present();
  CHECK(TlasContainsPool(data, rest_id), "%s: the next refit uses the promoted instance's live pose", label);
  CHECK(data->live.tlas_skips == skips, "%s: the promotion is not skipped (%u skips)", label, data->live.tlas_skips - skips);
  for (uint32_t p = 0; p <= bvh::kLiveTlasInterval; ++p) {
    MoveRefitInstance(dyn_id, 14.f + 0.05f * static_cast<float>(p), 12.f, frame());
    rig.present();
  }
  CHECK(data->live.tlas_rebuilds + data->live.tlas_skips > attempts, "%s: a rebuild follows the promotion", label);
  CHECK(TlasContainsPool(data, rest_id), "%s: the TLAS holds the promoted instance's live pose after the rebuild", label);
  CHECK(!TlasEntryRest(data, rest_id), "%s: after the rebuild the promoted entry is not a rest entry", label);
  bvh::g_live_bvh.slack.store(false);
}

static void TestRestPromotion() {
  RunRestPromotion(false, "promotion");
  RunRestPromotion(true, "promotion with slack");
}

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

// Pin of the rebuild path: a scripted scenario, with a forced rebuild after each step. Hashed after every rebuild.
// Pinned from HEAD behaviour; not re-pinned.
static uint64_t RebuildStateHash(bvh::BvhDeviceData* data, uint64_t h) {
  h = RefitStateHash(data, h);
  for (const bvh::LiveTlasInstance& t : data->tlas_info) {
    HashBytes(&h, &t.alpha, sizeof t.alpha);
    HashBytes(&h, &t.material, sizeof t.material);
    HashBytes(&h, &t.dynamic, sizeof t.dynamic);
    HashBytes(&h, &t.wind, sizeof t.wind);
    HashBytes(&h, &t.source, sizeof t.source);
    HashBytes(&h, t.inverse_world, sizeof t.inverse_world);
    HashBytes(&h, &t.visibility.camera_seen, sizeof t.visibility.camera_seen);
    HashBytes(&h, &t.visibility.light_seen, sizeof t.visibility.light_seen);
    HashBytes(&h, &t.visibility.near_fade, sizeof t.visibility.near_fade);
  }
  HashBytes(&h, &data->tlas_dynamic_first, sizeof data->tlas_dynamic_first);
  const bvh::LiveBvhStats& s = data->live;
  const uint32_t counters[] = {s.tlas_waiting, s.tlas_unusable, s.tlas_alpha_instances, s.tlas_alpha_waiting, s.tlas_near_fade,
                               s.tlas_camera_hidden, s.refit_rest_fast, s.refit_rest_slow, bvh::g_pool.stats.wind_sway_instances};
  HashBytes(&h, counters, sizeof counters);
  return h;
}

static void TestRebuildGolden() {
  SkipRig rig;
  bvh::BvhDeviceData* data = rig.data();
  bvh::g_live_bvh.skip_noop.store(false);  // every forced rebuild is built, none skipped as a no-op
  const uint64_t kA = 0xF1u, kS = 0xF2u, kW = 0xF3u, kR = 0xF4u;
  AddMesh(kA, BoxMesh(0.8f));
  AddMesh(kS, BoxMesh(0.5f));
  AddMesh(kW, Grid(5, 5, 2.f, 0.4f));
  AddMesh(kR, BoxMesh(1.1f));
  for (int i = 0; i < 6; ++i) {
    AddInstance(kA, 2.f * static_cast<float>(i), 0.f, 12.f);
    AddInstance(kS, 2.f * static_cast<float>(i) + 1.f, 0.f, 20.f);
    AddInstance(kW, 2.f * static_cast<float>(i), 0.f, 30.f);
    AddInstance(kR, 2.f * static_cast<float>(i), 0.f, 40.f);
  }
  uint64_t g = 14695981039346656037ull;
  const auto frame = [] { return falcom_world::g_state.frame.load(); };
  const auto ids_of = [](uint64_t key) {
    std::vector<uint64_t> ids;
    std::lock_guard lock(bvh::g_pool.mutex);
    for (const bvh::WorldInstance& inst : bvh::g_pool.instances) {
      if (inst.mesh_key == key) ids.push_back(inst.id);
    }
    return ids;
  };
  const auto rebuild = [&](const char* step) {  // the next TLAS update is a full rebuild ("refit failed"), no change needed
    const uint32_t before = data->live.tlas_rebuilds;
    data->tlas_refit_failed = true;
    rig.present();
    CHECK(data->live.tlas_rebuilds == before + 1u, "step %s: forced rebuild (reason %s)", step, data->live.tlas_reason);
    g = RebuildStateHash(data, g);
    std::printf("rebuild golden %s: %016llx\n", step, (unsigned long long)g);
  };
  const auto refit = [&](int presents) {
    for (int i = 0; i < presents; ++i) rig.present();
  };

  rig.present();
  rebuild("admits");

  RetireMesh(kR);
  refit(2);
  rebuild("retire and compact");

  const std::vector<uint64_t> a_ids = ids_of(kA);
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    const bvh::WorldInstance* inst = PoolInstance(a_ids[0]);
    const auto observed = bvh::g_pool.observations.find(bvh::InstanceKey{inst->mesh_key, falcom_world::MatrixHash(inst->matrix, 12)});
    observed->second.light_frames = observed->second.light_frames == 0u ? 1u : 0u;
    bvh::NotePoolVisibilityChange();
    FlipCameraSighting(a_ids[1]);
  }
  refit(2);
  rebuild("camera and light visibility flips");

  {
    std::lock_guard lock(bvh::g_pool.mutex);
    const bvh::WorldInstance* inst = PoolInstance(a_ids[2]);
    const auto observed = bvh::g_pool.observations.find(bvh::InstanceKey{inst->mesh_key, falcom_world::MatrixHash(inst->matrix, 12)});
    observed->second.camera_visibility.valid = true;
    observed->second.camera_visibility.color[0] = 0.25f;
    bvh::NotePoolVisibilityChange();
  }
  refit(2);
  rebuild("color-only input change");

  {
    std::lock_guard lock(bvh::g_pool.mutex);
    bvh::MarkPoolMeshAlpha(bvh::g_pool.mesh_by_key.at(kA), bvh::g_pool.alpha_keys[kA]);
    bvh::g_pool.dynamic_flag_revision += 1u;
  }
  refit(2);
  rebuild("alpha flag flip");

  {
    std::lock_guard lock(bvh::g_pool.mutex);
    bvh::g_pool.wind_keys.insert(kW);
    bvh::g_pool.wind_sway[kW] = RestWindSway(0);
  }
  refit(3);
  rebuild("wind key and sway added");

  const std::vector<uint64_t> s_ids = ids_of(kS);
  MoveRefitInstance(s_ids[0], 1.f, 20.f, frame());
  refit(3);
  rebuild("follow promotion");

  SetCamera(40.f, 5.f, 40.f);
  refit(2);
  rebuild("camera move changes the region");

  {
    std::lock_guard lock(bvh::g_pool.mutex);
    bvh::g_pool.alpha_wind_sway.store(false);
  }
  refit(3);
  rebuild("sway switch off");
  bvh::g_pool.alpha_wind_sway.store(true);
  refit(3);
  rebuild("sway switch on");

  {
    std::lock_guard lock(bvh::g_pool.mutex);
    for (bvh::WorldInstance& inst : bvh::g_pool.instances) {
      if (inst.id == s_ids[0]) inst.bounds_min[0] = std::nanf("");
    }
    bvh::g_pool.dynamic_revision += 1u;
  }
  refit(1);
  CHECK(data->tlas_refit_failed && data->live.refit_failures == 1u, "refit failure recorded (failures %u)", data->live.refit_failures);
  MoveRefitInstance(s_ids[0], 1.f, 20.f, frame());
  rebuild("failed refit");

  MoveStaticInstances(kS, 0.5f);
  rebuild("static pose edit");

  CHECK(g == 0x2c41a4dfd6f640d4ull, "rebuild golden equals HEAD pin (%016llx)", (unsigned long long)g);
}

// B3 carry-over: an organic rebuild keeps the visibility of the TLAS entries whose observation did not change and
// reads the touched ones from the pool. The changes go through ObservePoolInstance, as the draws make them.
static void TestVisibilityCarry() {
  SkipRig rig;
  bvh::BvhDeviceData* data = rig.data();
  bvh::g_live_bvh.skip_noop.store(false);  // every organic rebuild is built
  const uint64_t kA = 0x81u, kB = 0x82u;
  for (int i = 0; i < 4; ++i) {
    AddInstance(kA, 2.f * static_cast<float>(i), 0.f, 20.f);
    AddInstance(kB, 2.f * static_cast<float>(i), 0.f, 30.f);
  }
  std::vector<uint64_t> ids;
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    for (const bvh::WorldInstance& inst : bvh::g_pool.instances) ids.push_back(inst.id);
  }
  const uint64_t a = ids[0], b = ids[1], c = ids[2];
  const auto observe = [](uint64_t id, const bvh::PoolSighting& sighting, const bvh::PoolVisibility* vis) {
    std::lock_guard lock(bvh::g_pool.mutex);
    for (const bvh::WorldInstance& inst : bvh::g_pool.instances) {
      if (inst.id == id) bvh::ObservePoolInstance(inst.mesh_key, 0x1000u, inst.matrix, g_obs_frame++, &sighting, vis);
    }
  };
  const bvh::PoolSighting camera{static_cast<uint8_t>(bvh::kPoolPassCamera | bvh::kPoolPassVisibility), 0x5150u};
  const bvh::PoolSighting none{0u, 0u};
  bvh::PoolVisibility vis;
  vis.valid = true;
  vis.color[0] = 0.5f;
  vis.color[3] = 1.f;
  observe(b, camera, &vis);  // b has a camera view before the first build
  for (int i = 0; i < 3 && data->live.tlas_rebuilds == 0u; ++i) rig.present();
  CHECK(data->live.tlas_rebuilds == 1u && data->tlas_built, "first TLAS before the carry test (%u rebuilds)", data->live.tlas_rebuilds);

  const auto compare_all = [&](const char* label) {
    int compared = 0;
    std::lock_guard lock(bvh::g_pool.mutex);
    for (const bvh::LiveTlasInstance& t : data->tlas_info) {
      for (const bvh::WorldInstance& inst : bvh::g_pool.instances) {
        if (inst.id != t.id) continue;
        const bvh::PoolCameraVisibility fresh = bvh::GetPoolCameraVisibility(inst);
        const bvh::PoolCameraVisibility& got = t.visibility;
        CHECK(got.camera_seen == fresh.camera_seen, "%s instance %llu camera_seen %d vs fresh %d", label, (unsigned long long)t.id, got.camera_seen, fresh.camera_seen);
        CHECK(got.light_seen == fresh.light_seen, "%s instance %llu light_seen %d vs fresh %d", label, (unsigned long long)t.id, got.light_seen, fresh.light_seen);
        CHECK(got.near_fade == fresh.near_fade, "%s instance %llu near_fade %d vs fresh %d", label, (unsigned long long)t.id, got.near_fade, fresh.near_fade);
        CHECK(got.inputs.valid == fresh.inputs.valid, "%s instance %llu inputs.valid %d vs fresh %d", label, (unsigned long long)t.id, got.inputs.valid, fresh.inputs.valid);
        CHECK(std::memcmp(got.inputs.color, fresh.inputs.color, sizeof got.inputs.color) == 0, "%s instance %llu inputs.color %.3f vs fresh %.3f", label,
              (unsigned long long)t.id, (double)got.inputs.color[0], (double)fresh.inputs.color[0]);
        CHECK(std::memcmp(got.inputs.param, fresh.inputs.param, sizeof got.inputs.param) == 0, "%s instance %llu inputs.param", label, (unsigned long long)t.id);
        ++compared;
      }
    }
    CHECK(compared == static_cast<int>(data->tlas_info.size()), "%s every TLAS entry found in the pool (%d of %zu)", label, compared, data->tlas_info.size());
  };

  observe(a, camera, &vis);  // (a) camera-seen flips
  vis.color[0] = 0.25f;
  observe(b, camera, &vis);  // (b) only the color of the view changes
  observe(c, none, nullptr);  // (c) a sighting that changes nothing
  const uint32_t rebuilds = data->live.tlas_rebuilds;
  for (int i = 0; i < 16 && data->live.tlas_rebuilds == rebuilds; ++i) rig.present();
  CHECK(data->live.tlas_rebuilds == rebuilds + 1u && std::string(data->live.tlas_reason) == "visibility changed", "organic rebuild after the visibility changes (reason %s)", data->live.tlas_reason);
  compare_all("touched");
  CHECK(data->live.snapshot_reused > 0u, "untouched static instances are carried over (reused %u)", data->live.snapshot_reused);
  CHECK(data->live.snapshot_fresh > 0u, "touched instances are read fresh (fresh %u)", data->live.snapshot_fresh);
  std::printf("TestVisibilityCarry: rebuild 1 reused %u fresh %u\n", data->live.snapshot_reused, data->live.snapshot_fresh);

  BumpAdmit();  // no change to any observation: an organic rebuild of the same static set
  const uint32_t before = data->live.tlas_rebuilds;
  for (int i = 0; i < 16 && data->live.tlas_rebuilds == before; ++i) rig.present();
  CHECK(data->live.tlas_rebuilds == before + 1u && std::string(data->live.tlas_reason) == "pool changed", "second organic rebuild (reason %s)", data->live.tlas_reason);
  compare_all("unchanged");
  CHECK(data->live.snapshot_fresh == 0u, "no change: every static instance carried over (fresh %u)", data->live.snapshot_fresh);
  CHECK(data->live.snapshot_reused == data->tlas_info.size(), "no change: all %zu entries reused (reused %u)", data->tlas_info.size(), data->live.snapshot_reused);
  std::printf("TestVisibilityCarry: rebuild 2 reused %u fresh %u\n", data->live.snapshot_reused, data->live.snapshot_fresh);
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
  SetCamera(1200.f, 5.f, 10.f);  // the region (range 256 or 512) leaves most of the static block: the snapshot changes too
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
  // Apply cost (applies with ops > 0; a zero-op apply is counted as a skip in tlas.noop/skips, not here).
  CHECK(data->live.slack_apply_ms_sum > 0.f && data->live.slack_apply_ms_max > 0.f, "apply ms recorded after an apply (sum %.4f, max %.4f)", data->live.slack_apply_ms_sum, data->live.slack_apply_ms_max);
  CHECK(data->live.slack_apply_ms_max <= data->live.slack_apply_ms_sum, "apply ms: max %.4f <= sum %.4f", data->live.slack_apply_ms_max, data->live.slack_apply_ms_sum);
  const std::string text = ReadDumpText(bvh::WriteCacheDiagFile(data, 1u, "", bvh::CacheSwitches{}));
  const std::string slack = "\"slack\": {\"applies\": " + std::to_string(data->live.slack_applies) + ", \"apply_ms_sum\": ";
  CHECK(text.find(slack) != std::string::npos, "JSON slack block equals the live applies (%u)", data->live.slack_applies);
}

// ---------------------------------------------------------------------------
// BVH range: every pooled instance within R of the camera (plus the ray margin) is in the region.

static void RegionPresent(Device* dev, Queue* queue) {
  falcom_world::g_state.frame.fetch_add(1u);
  bvh::UpdateLiveBvh(dev, queue);
}

static void TestRegionCoverage(Device* dev, Queue* queue) {
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(dev);
  const float margin = bvh::kPoolRegionRayMargin;
  CHECK(margin == 10.f, "ray margin is the RtaoRayMax maximum (%.2f)", margin);
  const float ranges[3] = {64.f, 256.f, 512.f};
  const float cams[5][3] = {{10.f, 5.f, 10.f}, {255.99f, 0.f, -255.99f}, {256.f, 0.f, -256.f}, {-0.01f, 0.f, 0.01f}, {511.99f, 1000.f, 3.f}};
  for (float range : ranges) {
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      bvh::g_pool.region_size = range;
    }
    for (const auto& c : cams) {
      SetCamera(c[0], c[1], c[2]);
      const bvh::PoolRegion region = bvh::CurrentPoolRegion();
      CHECK(region.range == range && std::fabs(region.size - (3.f * range + 2.f * margin)) < 1e-3f, "R %.0f: box edge %.3f", range, region.size);
      for (int k = 0; k < 3; ++k) {
        CHECK(c[k] - region.min[k] >= range + margin - 1e-3f && region.max[k] - c[k] >= range + margin - 1e-3f,
              "R %.0f cam (%.2f %.2f %.2f) axis %d: gaps %.3f %.3f", range, c[0], c[1], c[2], k, c[k] - region.min[k], region.max[k] - c[k]);
      }
      auto probe = [&](const float* p) {
        bvh::WorldInstance point;
        for (int k = 0; k < 3; ++k) point.bounds_min[k] = point.bounds_max[k] = p[k];
        return bvh::PoolInstanceInRegion(point, region);
      };
      for (int axis = 0; axis < 3; ++axis) {
        for (float s : {-1.f, 1.f}) {
          float p[3] = {c[0], c[1], c[2]};
          p[axis] = c[axis] + s * range;
          CHECK(probe(p), "R %.0f: point at +-R on axis %d (%.2f)", range, axis, s);
          p[axis] = c[axis] + s * (range + margin);
          CHECK(probe(p), "R %.0f: point at +-(R+M) on axis %d (%.2f)", range, axis, s);
        }
      }
      for (int m = 0; m < 8; ++m) {
        const float p[3] = {c[0] + ((m & 1) ? range : -range), c[1] + ((m & 2) ? range : -range), c[2] + ((m & 4) ? range : -range)};
        CHECK(probe(p), "R %.0f: corner %d", range, m);
      }
      const float past[3] = {region.max[0] + 1.f, c[1], c[2]};
      CHECK(!probe(past), "R %.0f: one metre past the far face is outside", range);
      // Shader skip (world_rtao.cs_5_0.hlsl region_lo / region_hi), on the sphere of radius R and the cube corners.
      // Directions: 26 unit directions (points at distance R) and the 8 cube corners (per-axis R).
      int shader_outside = 0;
      for (int m = 0; m < 27; ++m) {
        if (m == 13) continue;
        const float dir[3] = {(m % 3 - 1) * 1.f, ((m / 3) % 3 - 1) * 1.f, ((m / 9) % 3 - 1) * 1.f};
        const float len = std::sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
        for (int k = 0; k < 3; ++k) {
          const float p = c[k] + range * dir[k] / len;
          if (p < region.min[k] + margin || p > region.min[k] + region.size - margin) ++shader_outside;
        }
      }
      CHECK(shader_outside == 0, "R %.0f: shader skip never reaches a sphere point (%d)", range, shader_outside);
    }
  }

  // Pool: six instances at +-R on each axis stay in the TLAS, and across a cell edge crossing.
  bvh::ResetWorldPool();
  RegionPresent(dev, queue);
  for (float range : ranges) {
    {
      std::lock_guard lock(bvh::g_pool.mutex);
      bvh::g_pool.region_size = range;
    }
    const uint64_t key = 0x100u + static_cast<uint64_t>(range);
    AddMesh(key, BoxMesh(1.f));
    const float x0 = range - 0.01f, y0 = 5.f, z0 = 10.f;
    SetCamera(x0, y0, z0);
    AddInstance(key, x0 + range, y0, z0);
    AddInstance(key, x0 - range, y0, z0);
    AddInstance(key, x0, y0 + range, z0);
    AddInstance(key, x0, y0 - range, z0);
    AddInstance(key, x0, y0, z0 + range);
    AddInstance(key, x0, y0, z0 - range);
    for (int i = 0; i < 40 && data->live.tlas_instances != 6u; ++i) RegionPresent(dev, queue);
    CHECK(data->live.tlas_instances == 6u, "R %.0f: six axis instances in the TLAS (%u)", range, data->live.tlas_instances);
    CHECK(data->live.tlas_instances == BuildRefInstances(*data, bvh::CurrentPoolRegion()).size(), "R %.0f: matches brute-force set (tlas %u refs %zu waiting %u reason %s)", range, data->live.tlas_instances, BuildRefInstances(*data, bvh::CurrentPoolRegion()).size(), data->live.tlas_waiting, data->live.tlas_reason);
    SetCamera(range + 0.01f, y0, z0);
    bool moved = false, all_kept = true;
    for (int i = 0; i < 10; ++i) {
      RegionPresent(dev, queue);
      moved = moved || std::string(data->live.tlas_reason) == "region moved";
      all_kept = all_kept && data->live.tlas_instances == 6u;
    }
    CHECK(moved, "R %.0f: the cell crossing rebuilt the TLAS (reason %s, tlas %u)", range, data->live.tlas_reason, data->live.tlas_instances);
    CHECK(all_kept && data->live.tlas_instances == 6u, "R %.0f: six instances kept across the crossing (%u)", range, data->live.tlas_instances);
    RetireMesh(key);
    for (int i = 0; i < 40 && data->live.tlas_instances != 0u; ++i) RegionPresent(dev, queue);
    CHECK(data->live.tlas_instances == 0u, "R %.0f: retired instances leave the TLAS (%u)", range, data->live.tlas_instances);
  }
  bvh::ResetWorldPool();
  {
    std::lock_guard lock(bvh::g_pool.mutex);
    bvh::g_pool.region_size = 512.f;
  }
}

static bool Near(float a, float b, float tol) { return std::fabs(a - b) <= tol; }

// Performance tab window (perf_run.hpp UpdatePerfTab): CPU and GPU per present over the last kPerfTabWindow presents.
static void ResetPerfTab() {
  std::lock_guard<std::mutex> lock(bvh::g_perf.mutex);
  bvh::g_perf.tab = bvh::PerfTabState{};
}

static bvh::PerfTabView PerfTabNow() {
  std::lock_guard<std::mutex> lock(bvh::g_perf.mutex);
  return bvh::g_perf.tab.view;
}

static void TestPerfTab(Device* dev) {
  ResetPerfTab();
  bvh::PerfCounters c;
  bvh::UpdatePerfTab(c);
  {
    std::lock_guard<std::mutex> lock(bvh::g_perf.mutex);
    CHECK(bvh::g_perf.tab.count == 0u && std::isnan(bvh::g_perf.tab.view.cpu_now), "the first present only primes (count %zu)", bvh::g_perf.tab.count);
  }
  // CPU: rebuild +0.5 ms per present (cumulative) and hooks 0.3 ms per present (per present): 0.8 ms each present.
  for (int i = 0; i < 40; ++i) {
    c.cost[bvh::kPerfTlasRebuild] += 0.5;
    c.cost[bvh::kPerfPoolHooks] = 0.3;
    bvh::UpdatePerfTab(c);
  }
  bvh::PerfTabView v = PerfTabNow();
  CHECK(Near(static_cast<float>(v.cpu_now), 0.8f, 1e-4f) && Near(static_cast<float>(v.cpu_avg), 0.8f, 1e-4f), "40 presents at 0.8 ms (now %.4f avg %.4f)", v.cpu_now, v.cpu_avg);
  CHECK(std::isnan(v.gpu_now) && std::isnan(v.gpu_avg), "no GPU sample in the window is --");
  // Ten presents at 1.8 ms: the window of 30 holds 20 at 0.8 and 10 at 1.8.
  for (int i = 0; i < 10; ++i) {
    c.cost[bvh::kPerfTlasRebuild] += 1.5;
    c.cost[bvh::kPerfPoolHooks] = 0.3;
    bvh::UpdatePerfTab(c);
  }
  v = PerfTabNow();
  CHECK(Near(static_cast<float>(v.cpu_now), 1.8f, 1e-4f) && Near(static_cast<float>(v.cpu_avg), (20.f * 0.8f + 10.f * 1.8f) / 30.f, 1e-4f),
        "ring wraps: now %.4f avg %.4f (want 1.8 and %.4f)", v.cpu_now, v.cpu_avg, (20.f * 0.8f + 10.f * 1.8f) / 30.f);
  // A cumulative counter below its previous value (reset) counts its new value: 0.2 + 0.3.
  c.cost[bvh::kPerfTlasRebuild] = 0.2;
  bvh::UpdatePerfTab(c);
  v = PerfTabNow();
  CHECK(Near(static_cast<float>(v.cpu_now), 0.5f, 1e-4f), "counter reset takes the new value (%.4f)", v.cpu_now);

  // GPU: trace +2.0 and RTAO +1.0 (one sample each) in one present: 3.0 ms.
  ResetPerfTab();
  c = bvh::PerfCounters{};
  bvh::UpdatePerfTab(c);
  c.cost[bvh::kPerfGpuTrace] += 2.0;
  c.events[bvh::kPerfGpuTrace] += 1u;
  c.cost[bvh::kPerfGpuRtao] += 1.0;
  c.events[bvh::kPerfGpuRtao] += 1u;
  bvh::UpdatePerfTab(c);
  v = PerfTabNow();
  CHECK(Near(static_cast<float>(v.gpu_now), 3.f, 1e-4f) && Near(static_cast<float>(v.gpu_avg), 3.f, 1e-4f), "trace 2 + RTAO 1 in one present = 3 (%.4f)", v.gpu_now);
  // One timer with 2 samples and 4.0 ms: 2.0 ms per sample; the average is over measured presents (3 and 2).
  c.cost[bvh::kPerfGpuAlphaBlit] += 4.0;
  c.events[bvh::kPerfGpuAlphaBlit] += 2u;
  bvh::UpdatePerfTab(c);
  v = PerfTabNow();
  CHECK(Near(static_cast<float>(v.now[bvh::kPerfGpuAlphaBlit]), 2.f, 1e-4f) && Near(static_cast<float>(v.gpu_now), 5.f, 1e-4f) && Near(static_cast<float>(v.gpu_avg), 5.f, 1e-4f),
        "2 samples of 4 ms = 2 per sample; the GPU total is the sum of the rows (now %.4f avg %.4f)", v.gpu_now, v.gpu_avg);
  bvh::UpdatePerfTab(c);  // no sample: not measured, each row keeps its newest measured value
  v = PerfTabNow();
  CHECK(Near(static_cast<float>(v.gpu_now), 5.f, 1e-4f) && Near(static_cast<float>(v.gpu_avg), 5.f, 1e-4f), "a present without samples is not averaged (now %.4f avg %.4f)", v.gpu_now, v.gpu_avg);
  for (int i = 0; i < 30; ++i) bvh::UpdatePerfTab(c);
  v = PerfTabNow();
  CHECK(std::isnan(v.gpu_now) && std::isnan(v.gpu_avg), "30 presents without samples: -- (now %.4f avg %.4f)", v.gpu_now, v.gpu_avg);

  // Maxima: rebuild +6.0 ms at present 20 and hooks 9.3 ms at present 35 (the rest 0.5 and 0.3); a 40 ms frame at 35 and a NaN
  // frame at 36. Present 15 has a 2.0 ms bracket against 0.5 ms of InPresent rows: unattributed 1.5. The presents after it have
  // negative raw values (clamped to 0), which must not lower the unattributed maximum. The CPU total maximum is the largest
  // per-sample total (9.8), not the sum of the row maxima (15.3).
  ResetPerfTab();
  c = bvh::PerfCounters{};
  bvh::UpdatePerfTab(c);
  for (int i = 0; i < 40; ++i) {
    c.cost[bvh::kPerfTlasRebuild] += i == 20 ? 6.0 : 0.5;
    c.cost[bvh::kPerfPoolHooks] = i == 35 ? 9.3 : 0.3;
    if (i == 15) c.cost[bvh::kPerfPresentWorldCpu] += 2.0;
    bvh::UpdatePerfTab(c, i == 35 ? 40.0 : i == 36 ? std::numeric_limits<double>::quiet_NaN() : 16.0);
  }
  v = PerfTabNow();
  CHECK(Near(static_cast<float>(v.max.row[bvh::kPerfPoolHooks]), 9.3f, 1e-4f) && Near(static_cast<float>(v.max.row[bvh::kPerfTlasRebuild]), 6.0f, 1e-4f),
        "row maxima (hooks %.4f, rebuild %.4f)", v.max.row[bvh::kPerfPoolHooks], v.max.row[bvh::kPerfTlasRebuild]);
  CHECK(Near(static_cast<float>(v.max.cpu), 9.8f, 1e-4f) && Near(static_cast<float>(v.max.frame), 40.f, 1e-4f) && Near(static_cast<float>(v.max.unattr), 1.5f, 1e-4f),
        "CPU total maximum 9.8 (not 15.3), frame maximum 40 (NaN skipped), unattributed maximum 1.5 kept (cpu %.4f frame %.4f unattr %.4f)", v.max.cpu, v.max.frame, v.max.unattr);
  for (int i = 0; i < 30; ++i) {
    c.cost[bvh::kPerfTlasRebuild] += 0.5;
    c.cost[bvh::kPerfPoolHooks] = 0.3;
    bvh::UpdatePerfTab(c, 16.0);
  }
  v = PerfTabNow();
  CHECK(Near(static_cast<float>(v.max.cpu), 0.8f, 1e-4f) && Near(static_cast<float>(v.max.row[bvh::kPerfPoolHooks]), 0.3f, 1e-4f) && Near(static_cast<float>(v.max.frame), 16.f, 1e-4f),
        "after 30 more presents the spike is gone (cpu %.4f hooks %.4f frame %.4f)", v.max.cpu, v.max.row[bvh::kPerfPoolHooks], v.max.frame);

  // A Gpu row maximum is over the presents with a sample: trace 2 ms (one sample), none, then 5 ms (one sample).
  ResetPerfTab();
  c = bvh::PerfCounters{};
  bvh::UpdatePerfTab(c);
  c.cost[bvh::kPerfGpuTrace] += 2.0;
  c.events[bvh::kPerfGpuTrace] += 1u;
  bvh::UpdatePerfTab(c);
  bvh::UpdatePerfTab(c);
  c.cost[bvh::kPerfGpuTrace] += 5.0;
  c.events[bvh::kPerfGpuTrace] += 1u;
  bvh::UpdatePerfTab(c);
  v = PerfTabNow();
  CHECK(Near(static_cast<float>(v.max.row[bvh::kPerfGpuTrace]), 5.f, 1e-4f) && Near(static_cast<float>(v.max.gpu), 5.f, 1e-4f),
        "GPU maximum over the measured presents (trace %.4f, total %.4f)", v.max.row[bvh::kPerfGpuTrace], v.max.gpu);
  CHECK(std::isnan(v.max.row[bvh::kPerfGpuRtaoFilter]) && std::isnan(v.max.row[bvh::kPerfGpuAlphaBlit]), "a Gpu row never measured is --");

  // The live path feeds the tab at every present (SamplePerfPresent), with the pool lock released by the caller.
  ResetPerfTab();
  bvh::SamplePerfPresent(dev);
  bvh::SamplePerfPresent(dev);
  {
    std::lock_guard<std::mutex> lock(bvh::g_perf.mutex);
    CHECK(bvh::g_perf.tab.count == 1u, "SamplePerfPresent feeds the window (count %zu)", bvh::g_perf.tab.count);
  }
  CHECK(g_under_lock == 0, "the tab takes no lock the caller holds (%d)", g_under_lock);
  ResetPerfTab();
}

// The unattributed CPU row: the two present brackets minus the rows inside them, clamped at zero. The CPU total is the sum
// of the rows shown; the GPU total is the sum of the GPU rows that have a value.
static void TestPerfUnattributed() {
  ResetPerfTab();
  bvh::PerfCounters c;
  bvh::UpdatePerfTab(c);
  c.cost[bvh::kPerfPresentWorldCpu] += 10.0;
  c.cost[bvh::kPerfMeshCapture] = 7.0;
  bvh::UpdatePerfTab(c);
  bvh::PerfTabView v = PerfTabNow();
  CHECK(Near(static_cast<float>(v.unattr_now), 3.f, 1e-4f) && Near(static_cast<float>(v.cpu_now), 10.f, 1e-4f),
        "bracket 10 ms and rows 7 ms: unattributed 3, CPU total 10 (%.4f, %.4f)", v.unattr_now, v.cpu_now);
  c.cost[bvh::kPerfPresentWorldCpu] += 10.0;
  c.cost[bvh::kPerfMeshCapture] = 12.0;
  bvh::UpdatePerfTab(c);
  v = PerfTabNow();
  CHECK(v.unattr_now == 0.0 && v.unattr_raw_now < -0.01 && Near(static_cast<float>(v.cpu_now), 12.f, 1e-4f),
        "rows over the bracket clamp to 0 and are flagged (raw %.4f, CPU total %.4f)", v.unattr_raw_now, v.cpu_now);
  c.cost[bvh::kPerfPresentWorldCpu] += 10.0;
  c.cost[bvh::kPerfMeshCapture] = 10.004;
  bvh::UpdatePerfTab(c);
  v = PerfTabNow();
  CHECK(v.unattr_now == 0.0 && v.unattr_raw_now < 0.0 && v.unattr_raw_now > -0.01, "a 4 us overrun clamps without the flag (raw %.4f)", v.unattr_raw_now);
  c.cost[bvh::kPerfGpuTrace] += 2.0;
  c.events[bvh::kPerfGpuTrace] += 1u;
  c.cost[bvh::kPerfGpuRtao] += 1.0;
  c.events[bvh::kPerfGpuRtao] += 1u;
  bvh::UpdatePerfTab(c);
  v = PerfTabNow();
  CHECK(Near(static_cast<float>(v.gpu_now), 3.f, 1e-4f) && std::isnan(v.now[bvh::kPerfGpuAlphaBlit]),
        "GPU total is the sum of the rows with a value (%.4f)", v.gpu_now);
  ResetPerfTab();
}

// The timer that feeds every CPU stopwatch adds nothing while gathering is off and the measured time while it is on.
static void TestPerfTimerGate() {
  std::atomic_uint64_t total{0u};
  bvh::g_perf_gate.gather.store(false);
  {
    const bvh::PoolCpuTimer timer{&total};
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  CHECK(total.load() == 0u, "PoolCpuTimer with gathering off adds nothing (%llu us)", (unsigned long long)total.load());
  bvh::g_perf_gate.gather.store(true);
  {
    const bvh::PoolCpuTimer timer{&total};
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  CHECK(total.load() >= 1000u, "PoolCpuTimer with gathering on adds the scope time (%llu us)", (unsigned long long)total.load());
}

// The drain row is the drain minus the capture inside it, so the capture is counted once; the tab total counts it once.
static void TestPerfCaptureInDrain(Device* dev) {
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(dev);
  bvh::g_perf_gate.gather.store(true);
  bvh::g_pool.cpu_drain_frame_us.store(5000u);
  bvh::g_pool.cpu_capture_frame_us.store(2000u);
  bvh::g_pool.cpu_hook_frame_us.store(0u);
  bvh::g_deform_live.cpu_frame_us.store(0u);
  bvh::PerfCounters c;
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    c = bvh::ReadPerfCounters(data);
  }
  const double excl = c.cost[bvh::kPerfPoolDrainExclCapture];
  const double capture = c.cost[bvh::kPerfMeshCapture];
  CHECK(Near(static_cast<float>(excl), 3.f, 1e-4f) && Near(static_cast<float>(capture), 2.f, 1e-4f) && Near(static_cast<float>(excl + capture), 5.f, 1e-4f),
        "drain 5 ms with capture 2 ms: excluding row %.4f, capture %.4f", excl, capture);
  CHECK(excl >= 0.0, "the drain row is never negative (%.4f)", excl);
  ResetPerfTab();
  bvh::UpdatePerfTab(c);  // primes
  bvh::UpdatePerfTab(c);  // same counters: cumulative rows add nothing, the per-present rows are this present
  const bvh::PerfTabView v = PerfTabNow();
  CHECK(Near(static_cast<float>(v.cpu_now), 5.f, 1e-4f), "the tab counts the drain once (CPU total %.4f)", v.cpu_now);
  bvh::g_pool.cpu_drain_frame_us.store(0u);
  bvh::g_pool.cpu_capture_frame_us.store(0u);
  ResetPerfTab();
}

// A block added during present N lands in present N's sample: the sample at the start of N+1 holds it, the one after does not.
static void TestPerfSampleAlignment(Device* dev) {
  bvh::g_perf_gate.gather.store(true);
  ResetPerfTab();
  bvh::SamplePerfPresent(dev);  // primes
  bvh::g_perf_gate.present_world_us.fetch_add(10000u);  // 10 ms inside present N
  bvh::SamplePerfPresent(dev);  // start of N+1: the sample of N
  CHECK(Near(static_cast<float>(PerfTabNow().now[bvh::kPerfPresentWorldCpu]), 10.f, 1e-4f),
        "a 10 ms block in present N is in N's sample (%.4f)", PerfTabNow().now[bvh::kPerfPresentWorldCpu]);
  bvh::SamplePerfPresent(dev);  // present N+1 adds nothing
  CHECK(Near(static_cast<float>(PerfTabNow().now[bvh::kPerfPresentWorldCpu]), 0.f, 1e-4f),
        "the next sample does not carry it (%.4f)", PerfTabNow().now[bvh::kPerfPresentWorldCpu]);
  ResetPerfTab();
}

// A block adds time only while gathering is ON: AddPerfBlockMs and the timer add nothing with the gate OFF.
static void TestPerfBlockGate() {
  bvh::g_perf_gate.gather.store(false);
  bvh::AddPerfBlockMs(bvh::kPerfBlockSyncMeshes, 2.0);
  {
    const bvh::PoolCpuTimer timer{&bvh::g_perf_gate.block_us[bvh::kPerfBlockCacheDiag]};
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  uint64_t off_total = 0u;
  for (const std::atomic_uint64_t& us : bvh::g_perf_gate.block_us) off_total += us.load();
  CHECK(off_total == 0u, "gathering off: no block time is added (%llu us)", (unsigned long long)off_total);
  bvh::g_perf_gate.gather.store(true);
  bvh::AddPerfBlockMs(bvh::kPerfBlockSyncMeshes, 2.0);
  {
    const bvh::PoolCpuTimer timer{&bvh::g_perf_gate.block_us[bvh::kPerfBlockCacheDiag]};
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  const uint64_t sync = bvh::g_perf_gate.block_us[bvh::kPerfBlockSyncMeshes].load();
  const uint64_t diag = bvh::g_perf_gate.block_us[bvh::kPerfBlockCacheDiag].load();
  CHECK(sync == 2000u && diag >= 1000u, "gathering on: AddPerfBlockMs adds 2 ms and the scope adds its time (%llu, %llu us)",
        (unsigned long long)sync, (unsigned long long)diag);
  for (auto& us : bvh::g_perf_gate.block_us) us.store(0u);
}

// Each block key lands in its own sample: one block at a time, the sample of that present holds only that row.
static void TestPerfBlockSample(Device* dev) {
  bvh::g_perf_gate.gather.store(true);
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(dev);
  for (auto& us : bvh::g_perf_gate.block_us) us.store(0u);
  ResetPerfTab();
  bvh::PerfCounters c;
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    c = bvh::ReadPerfCounters(data);
  }
  bvh::UpdatePerfTab(c);  // primes
  bool all_own = true;
  for (size_t k = 0; k < bvh::kPerfBlockCount; ++k) {
    bvh::AddPerfBlockMs(static_cast<bvh::PerfBlock>(k), 1.0);
    {
      std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
      c = bvh::ReadPerfCounters(data);
    }
    bvh::UpdatePerfTab(c);
    const bvh::PerfTabView v = PerfTabNow();
    for (size_t row = bvh::kPerfBlockRow; row < bvh::kPerfCostCount; ++row) {
      const double want = row == bvh::kPerfBlockRow + k ? 1.0 : 0.0;
      if (!Near(static_cast<float>(v.now[row]), static_cast<float>(want), 1e-4f)) all_own = false;
    }
  }
  CHECK(all_own, "each block row holds only its own 1 ms in its own sample");
  for (auto& us : bvh::g_perf_gate.block_us) us.store(0u);
  ResetPerfTab();
}

// Disjoint blocks (InPresent) reduce the unattributed CPU; subset blocks are not summed.
static void TestPerfDisjointBlocks() {
  double values[bvh::kPerfCostCount] = {};
  values[bvh::kPerfPresentBvhCpu] = 10.0;
  values[bvh::kPerfBlockRow + bvh::kPerfBlockCacheDiag] = 4.0;
  const bvh::PerfTotalsValue before = bvh::PerfTotals(values);
  values[bvh::kPerfBlockRow + bvh::kPerfBlockRebuildCall] = 6.0;  // subset: not in the totals
  const bvh::PerfTotalsValue after = bvh::PerfTotals(values);
  CHECK(Near(static_cast<float>(before.unattr), 6.f, 1e-4f) && Near(static_cast<float>(before.cpu), 10.f, 1e-4f),
        "bracket 10 ms, disjoint block 4 ms: unattributed 6, CPU total 10 (%.4f, %.4f)", before.unattr, before.cpu);
  CHECK(Near(static_cast<float>(after.unattr), 6.f, 1e-4f) && Near(static_cast<float>(after.cpu), 10.f, 1e-4f),
        "a subset block of 6 ms changes neither the unattributed value nor the CPU total (%.4f, %.4f)", after.unattr, after.cpu);
}

// A 10 ms block in one present is the largest block of the attribution (a 3 ms block is over 2 ms, the largest is not).
static void TestPerfBlockAttribution(Device* dev) {
  bvh::ResetCacheDiagCounters();
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(dev);
  uint32_t saved_start = 0u;
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    saved_start = bvh::g_pool.episode_start_frame;
    bvh::g_pool.episode_start_frame = 0u;
  }
  bvh::g_perf_gate.gather.store(true);
  for (auto& us : bvh::g_perf_gate.block_us) us.store(0u);
  const auto base = std::chrono::steady_clock::now();
  const bvh::CacheSwitches sw{true, false, false, true};
  bvh::RequestPerfRun("attribution");
  bvh::UpdatePerfRun(data, 9000u, base, sw);
  int k = 0;
  while (bvh::g_perf.active.load() && k < 12000) {
    ++k;
    if (k == 100) bvh::AddPerfBlockMs(bvh::kPerfBlockSyncMeshes, 10.0);
    if (k == 200) bvh::AddPerfBlockMs(bvh::kPerfBlockDeformProbe, 3.0);
    bvh::UpdatePerfRun(data, 9000u + static_cast<uint32_t>(k), base + std::chrono::milliseconds(16 * k), sw);
  }
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    bvh::g_pool.episode_start_frame = saved_start;
  }
  const std::string text = ReadDumpText(WrittenPath(PerfResult()));
  CHECK(JsonError(text).empty(), "attribution perf file is valid JSON: %s", JsonError(text).c_str());
  const size_t attr = text.find("\"attribution\": {");
  CHECK(attr != std::string::npos, "perf file has the attribution object");
  const std::string tail = text.substr(attr);
  CHECK(std::fabs(ValueAt(tail, "\"sync_live_meshes\"", "\"max_per_present\"") - 10.0) <= 1e-4, "the 10 ms block is the sync block maximum");
  CHECK(std::fabs(ValueAt(tail, "\"deform_probe\"", "\"max_per_present\"") - 3.0) <= 1e-4, "the 3 ms block is the deform probe maximum");
  CHECK(ValueAt(tail, "\"sync_live_meshes\"", "\"presents_over_2ms\"") == 1.0, "one present over 2 ms for the sync block");
  CHECK(ValueAt(tail, "\"deform_probe\"", "\"presents_over_2ms\"") == 1.0, "one present over 2 ms for the deform probe block");
  CHECK(ValueAt(tail, "\"rebuild_call\"", "\"presents_over_2ms\"") == 0.0, "an idle block is never over 2 ms");
  CHECK(text.find("\"sync_live_meshes\": {\"max_per_present\": {\"value\": 10, \"unit\": \"ms/present\"}") != std::string::npos,
        "the stats row shows the same 10 ms maximum");
  bvh::ResetCacheDiagCounters();
}

// The i-th number of a series array (after the anchor and key); NaN when absent or null.
static double SeriesAt(const std::string& text, const char* anchor, const char* key, size_t i) {
  const size_t at = FindAfter(text, anchor, key);
  if (at == std::string::npos) return std::numeric_limits<double>::quiet_NaN();
  size_t p = text.find('[', at) + 1u;
  for (size_t n = 0; n < i; ++n) p = text.find(',', p) + 1u;
  if (text.compare(p, 4, "null") == 0) return std::numeric_limits<double>::quiet_NaN();
  return std::strtod(text.c_str() + p, nullptr);
}

// The worst frame interval (60 ms at present 200) and the worst raw unattributed present (present 100: 14 ms bracket, 10 ms block).
static void TestPerfWorstPresent(Device* dev) {
  bvh::ResetCacheDiagCounters();
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(dev);
  uint32_t saved_start = 0u;
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    saved_start = bvh::g_pool.episode_start_frame;
    bvh::g_pool.episode_start_frame = 0u;
  }
  bvh::g_perf_gate.gather.store(true);
  bvh::ClearPerfGathered(data);
  const auto base = std::chrono::steady_clock::now();
  const bvh::CacheSwitches sw{true, false, false, true};
  bvh::RequestPerfRun("worst");
  bvh::UpdatePerfRun(data, 9000u, base, sw);
  int k = 0;
  while (bvh::g_perf.active.load() && k < 12000) {
    ++k;
    if (k == 100) {
      bvh::AddPerfBlockMs(bvh::kPerfBlockResolveIndirect, 10.0);
      bvh::g_perf_gate.present_world_us.fetch_add(14000u);
    }
    if (k == 200) {
      bvh::AddPerfBlockMs(bvh::kPerfBlockSyncMeshes, 2.5);
      bvh::AddPerfBlockMs(bvh::kPerfBlockDeformProbe, 1.5);
    }
    bvh::UpdatePerfRun(data, 9000u + static_cast<uint32_t>(k), base + std::chrono::milliseconds(16 * k + (k >= 200 ? 44 : 0)), sw);
  }
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    bvh::g_pool.episode_start_frame = saved_start;
  }
  const std::string text = ReadDumpText(WrittenPath(PerfResult()));
  CHECK(JsonError(text).empty(), "worst present perf file is valid JSON: %s", JsonError(text).c_str());
  const char* frame_anchor = "\"worst_present\": {";
  const char* unattr_anchor = "\"worst_unattributed_present\": {";
  CHECK(text.find(frame_anchor) != std::string::npos && text.find(unattr_anchor) != std::string::npos, "both worst presents are written");
  CHECK(ValueAt(text, frame_anchor, "\"index\"") == 199.0, "worst frame is series index 199 (%.0f)", ValueAt(text, frame_anchor, "\"index\""));
  CHECK(ValueAt(text, frame_anchor, "\"frame\"") == 9200.0, "worst frame is frame 9200");
  CHECK(std::fabs(ValueAt(text, frame_anchor, "\"frame_ms\"") - 60.0) <= 1e-4, "worst frame interval 60 ms");
  CHECK(std::fabs(ValueAt(text, frame_anchor, "\"sync_live_meshes\"") - 2.5) <= 1e-4, "worst frame block sync 2.5 ms");
  CHECK(std::fabs(ValueAt(text, frame_anchor, "\"deform_probe\"") - 1.5) <= 1e-4, "worst frame block deform probe 1.5 ms");
  CHECK(std::fabs(ValueAt(text, frame_anchor, "\"blocks_in_present_ms\"") - 4.0) <= 1e-4, "blocks in present 4 ms");
  CHECK(FindAfter(text, frame_anchor, "\"in_series\": true") != std::string::npos, "the worst frame is in the series");
  CHECK(ValueAt(text, unattr_anchor, "\"index\"") == 99.0, "worst unattributed present is series index 99");
  CHECK(std::fabs(ValueAt(text, unattr_anchor, "\"unattributed_raw_ms\"") - 4.0) <= 1e-4, "its raw unattributed is 14 - 10 = 4 ms");
  CHECK(std::fabs(ValueAt(text, unattr_anchor, "\"resolve_indirect\"") - 10.0) <= 1e-4, "its resolve_indirect block is 10 ms");
  CHECK(SeriesAt(text, "\"blocks_unit\"", "\"resolve_indirect\": ", 99) == 10.0, "series block resolve_indirect[99] is 10 ms");
  CHECK(SeriesAt(text, "\"blocks_unit\"", "\"resolve_indirect\": ", 100) == 0.0, "series block resolve_indirect[100] is 0 (a delta)");
  CHECK(SeriesAt(text, "\"blocks_unit\"", "\"sync_live_meshes\": ", 199) == 2.5, "series block sync_live_meshes[199] is 2.5 ms");
  CHECK(CountOf(text, "\"value\": ") == CountOf(text, "\"unit\": "), "every value object has a unit (worst present)");
  bvh::ResetCacheDiagCounters();
}

// The tab keeps the largest frame interval of its 30 presents, with its blocks, until 30 newer presents push it out.
static void TestPerfSpikeTab() {
  ResetPerfTab();
  bvh::PerfCounters c;
  bvh::UpdatePerfTab(c, 16.0);
  for (int i = 0; i < 30; ++i) {
    if (i == 10) c.cost[bvh::kPerfBlockRow + bvh::kPerfBlockResolveIndirect] += 6.0;
    bvh::UpdatePerfTab(c, i == 10 ? 50.0 : 16.0);
  }
  bvh::PerfTabView v = PerfTabNow();
  CHECK(v.spike.valid && std::fabs(v.spike.frame_ms - 50.0) <= 1e-9, "tab spike is the 50 ms present (%.3f)", v.spike.frame_ms);
  CHECK(v.spike_age == 19u, "tab spike is 19 presents ago (%zu)", v.spike_age);
  CHECK(std::fabs(v.spike.value[bvh::kPerfBlockRow + bvh::kPerfBlockResolveIndirect] - 6.0) <= 1e-9, "tab spike block is 6 ms");
  CHECK(std::fabs(v.spike.cpu_total - 6.0) <= 1e-9, "tab spike CPU total is its block (%.4f)", v.spike.cpu_total);
  for (int i = 0; i < 30; ++i) bvh::UpdatePerfTab(c, 16.0);
  v = PerfTabNow();
  CHECK(v.spike.valid && std::fabs(v.spike.frame_ms - 16.0) <= 1e-9, "the 50 ms present left the window (%.3f)", v.spike.frame_ms);
  ResetPerfTab();
}

// Settings: the live parts of each setting are the sums of its rows per present; the maximum is the largest sum of one present.
static void TestPerfSettingsCost(Device* dev) {
  bvh::ResetCacheDiagCounters();
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(dev);
  uint32_t saved_start = 0u;
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    saved_start = bvh::g_pool.episode_start_frame;
    bvh::g_pool.episode_start_frame = 0u;
  }
  bvh::g_perf_gate.gather.store(true);
  bvh::ClearPerfGathered(data);
  const auto base = std::chrono::steady_clock::now();
  const bvh::CacheSwitches sw{true, false, false, true};
  bvh::RequestPerfRun("settings");
  bvh::UpdatePerfRun(data, 9000u, base, sw);
  int k = 0;
  while (bvh::g_perf.active.load() && k < 12000) {
    ++k;
    bvh::g_perf_gate.alpha_sync_us.fetch_add(1000u);
    data->alpha.timer.ms_sum += 0.5;
    data->alpha.timer.samples += 1u;
    if (k == 100) bvh::AddPerfBlockMs(bvh::kPerfBlockP2bRecord, 4.0);
    if (k == 200) {
      std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
      bvh::g_pool.stats.dynamic_expand_cpu_ms_sum += 3.f;
    }
    bvh::UpdatePerfRun(data, 9000u + static_cast<uint32_t>(k), base + std::chrono::milliseconds(16 * k), sw);
  }
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    bvh::g_pool.episode_start_frame = saved_start;
  }
  const std::string text = ReadDumpText(WrittenPath(PerfResult()));
  CHECK(JsonError(text).empty(), "settings perf file is valid JSON: %s", JsonError(text).c_str());
  const double presents = ValueAt(text, "\"window\": {", "\"presents\"");
  const char* alpha = "\"alpha_foliage\": {\"switch_keys\"";
  const char* follow = "\"follow_moving\": {\"switch_keys\"";
  CHECK(std::fabs(ValueAt(text, alpha, "\"avg_per_present\"") - ValueAt(text, "\"alpha_sync_cpu\": {", "\"avg_per_present\"")) <= 1e-6,
        "alpha foliage live CPU average is the alpha sync row");
  CHECK(std::fabs(ValueAt(text, alpha, "\"avg_per_present\"") - 1.0) <= 1e-4, "alpha foliage live CPU average is 1 ms per present");
  CHECK(std::fabs(ValueAt(text, alpha, "\"avg_per_measured_frame\"") - 0.5) <= 1e-4, "alpha foliage live GPU average is 0.5 ms");
  CHECK(std::fabs(ValueAt(text, follow, "\"max_per_present\"") - 4.0) <= 1e-4, "follow moving maximum is 4 ms (the largest present, not 4 + 3)");
  CHECK(std::fabs(ValueAt(text, follow, "\"avg_per_present\"") - 7.0 / presents) <= 1e-6, "follow moving average is (4 + 3) / presents");
  CHECK(std::fabs(ValueAt(text, follow, "\"avg_per_present\"")
                  - (ValueAt(text, "\"follow_scan\": {", "\"avg_per_present\"") + ValueAt(text, "\"p2b_expand_cpu\": {", "\"avg_per_present\"")
                     + ValueAt(text, "\"p2b_record\": {", "\"avg_per_present\"")))
            <= 1e-6,
        "follow moving average is the sum of its rows");
  CHECK(text.find("\"stable\": true}, \"settings\"") != std::string::npos, "unchanged switches: the inclusion switches are stable");
  CHECK(CountOf(text, "\"needs_ab\": \"") == bvh::kPerfSettingCount, "every setting names its A/B run");
  bvh::ResetCacheDiagCounters();
}

// Foliage motion has no live part: its cost is in the context rows and the A/B run.
static void TestPerfFoliageMotion() {
  const std::string text = ReadDumpText(WrittenPath(PerfResult()));
  const char* motion = "\"foliage_motion\": {\"switch_keys\"";
  CHECK(FindAfter(text, motion, "\"live_parts\": []") != std::string::npos, "foliage motion has no live part");
  CHECK(FindAfter(text, motion, "\"live_cpu\": {\"avg_per_present\": {\"value\": null") != std::string::npos, "foliage motion live CPU is null");
  CHECK(FindAfter(text, motion, "\"context_rows\": {\"tlas_refit\"") != std::string::npos, "foliage motion lists tlas_refit as context");
}

// A switch changed in the middle of the run: stable false, with the start and end values of that switch only.
static void TestPerfInclusionFlip(Device* dev) {
  bvh::ResetCacheDiagCounters();
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(dev);
  uint32_t saved_start = 0u;
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    saved_start = bvh::g_pool.episode_start_frame;
    bvh::g_pool.episode_start_frame = 0u;
  }
  bvh::g_perf_gate.gather.store(true);
  const auto base = std::chrono::steady_clock::now();
  const bvh::CacheSwitches sw{true, false, false, true};
  bvh::RequestPerfRun("flip");
  bvh::UpdatePerfRun(data, 9000u, base, sw);
  int k = 0;
  while (bvh::g_perf.active.load() && k < 12000) {
    ++k;
    if (k == 50) bvh::g_pool.scan_indirect.store(false);
    bvh::UpdatePerfRun(data, 9000u + static_cast<uint32_t>(k), base + std::chrono::milliseconds(16 * k), sw);
  }
  bvh::g_pool.scan_indirect.store(true);
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    bvh::g_pool.episode_start_frame = saved_start;
  }
  const std::string text = ReadDumpText(WrittenPath(PerfResult()));
  CHECK(JsonError(text).empty(), "flip perf file is valid JSON: %s", JsonError(text).c_str());
  CHECK(text.find("\"stable\": false}, \"settings\"") != std::string::npos, "a switch changed mid-run: not stable");
  const size_t start = text.find("\"start\": {", text.find("\"inclusion_switches\""));
  const size_t end = text.find("\"end\": {", start);
  const size_t stop = text.find("}, \"stable\"", end);
  const std::string start_part = text.substr(start, end - start);
  const std::string end_part = text.substr(end, stop - end);
  for (size_t i = 0; i < bvh::kPerfInclusionCount; ++i) {
    const std::string key = std::string("\"") + bvh::kPerfInclusionNames[i] + "\": ";
    const bool at_start = start_part.compare(start_part.find(key) + key.size(), 4, "true") == 0;
    const bool at_end = end_part.compare(end_part.find(key) + key.size(), 4, "true") == 0;
    if (std::string(bvh::kPerfInclusionNames[i]) == "scan_indirect") {
      CHECK(at_start && !at_end, "scan indirect is on at the start and off at the end");
    } else {
      CHECK(at_start == at_end, "switch %s is the same at both ends", bvh::kPerfInclusionNames[i]);
    }
  }
  bvh::ResetCacheDiagCounters();
}

// The frame values are exchanged by the sample (gather ON): each *_frame_us holds the cumulative value of the present
// that ended, the cumulative value restarts at 0. A deform update after the sample does not lose a fresh value.
static void TestPerfFrameExchange(Device* dev, Queue* queue) {
  bvh::g_perf_gate.gather.store(true);
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(dev);
  bvh::g_pool.cpu_drain_us.store(5000u);
  bvh::g_pool.cpu_capture_us.store(2000u);
  bvh::g_pool.cpu_hook_us.store(300u);
  bvh::g_deform_live.cpu_us.store(400u);
  bvh::SamplePerfPresent(dev);
  CHECK(bvh::g_pool.cpu_drain_frame_us.load() == 5000u && bvh::g_pool.cpu_capture_frame_us.load() == 2000u
            && bvh::g_pool.cpu_hook_frame_us.load() == 300u && bvh::g_deform_live.cpu_frame_us.load() == 400u,
        "the sample holds the frame values (drain %llu capture %llu hook %llu deform %llu)",
        (unsigned long long)bvh::g_pool.cpu_drain_frame_us.load(), (unsigned long long)bvh::g_pool.cpu_capture_frame_us.load(),
        (unsigned long long)bvh::g_pool.cpu_hook_frame_us.load(), (unsigned long long)bvh::g_deform_live.cpu_frame_us.load());
  CHECK(bvh::g_pool.cpu_drain_us.load() == 0u && bvh::g_pool.cpu_capture_us.load() == 0u && bvh::g_pool.cpu_hook_us.load() == 0u
            && bvh::g_deform_live.cpu_us.load() == 0u,
        "the cumulative values restart at 0 after the sample");
  const bool deform_was_on = bvh::g_deform_live.enabled.load();
  bvh::g_deform_live.enabled.store(false);  // off: no deform resources are made by this check
  bvh::g_deform_live.cpu_us.store(567u);
  bvh::UpdateDeformLive(dev, queue);
  bvh::g_deform_live.enabled.store(deform_was_on);
  CHECK(bvh::g_deform_live.cpu_us.load() == 567u && bvh::g_deform_live.cpu_frame_us.load() == 400u,
        "UpdateDeformLive leaves a fresh cumulative value alone (cpu_us %llu, frame %llu)",
        (unsigned long long)bvh::g_deform_live.cpu_us.load(), (unsigned long long)bvh::g_deform_live.cpu_frame_us.load());
  bvh::ClearPerfGathered(data);
}

// Gathering OFF gathers nothing: no tab sample, no sum, no stopwatch value, no frame value is exchanged. The switch's clear
// zeroes what was gathered and the dump says perf_gather false. The same sequence with gathering ON does gather.
static bool PerfSumsZero(bvh::BvhDeviceData* data) {
  std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
  const bvh::PoolTlasTiming& t = bvh::g_pool.tlas_timing;
  const bvh::LiveBvhStats& live = data->live;
  return t.rebuild_ms_sum == 0.f && t.refit_ms_sum == 0.f && t.after_drain_ms_sum == 0.f && t.rebuild_phase_ms_sum[2] == 0.f
         && t.refit_phase_ms_sum[2] == 0.f && live.blas_build_ms_sum == 0.f && live.slack_apply_ms_sum == 0.f
         && live.tlas_skip_ms_sum == 0.f && live.tlas_ms_last == 0.f && live.refit_ms_last == 0.f && live.mesh_ms_last == 0.f
         && bvh::g_pool.stats.dynamic_expand_cpu_ms_sum == 0.f;
}

static void TestPerfGateOff(Device* dev, Queue* queue) {
  bvh::g_perf_gate.gather.store(false);
  bvh::ClearPerfGathered(bvh::GetBvhDeviceData(dev));
  bvh::ResetCacheDiagCounters();
  ResetPerfTab();
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(dev);
  AddMesh(0x700u, BoxMesh(0.8f));
  AddInstance(0x700u, 120.f, 0.f, 120.f);
  bvh::g_pool.cpu_hook_frame_us.store(777u);
  bvh::g_pool.cpu_drain_frame_us.store(777u);
  bvh::g_pool.cpu_capture_frame_us.store(777u);
  bvh::g_deform_live.cpu_frame_us.store(777u);
  const uint64_t builds_before = data->live.blas_builds;
  for (int i = 0; i < 4; ++i) {
    falcom_world::g_state.frame.fetch_add(1u);
    bvh::SamplePerfPresent(dev);
    bvh::UpdateLiveBvh(dev, queue);
    bvh::UpdateCacheDiag(dev);
  }
  CHECK(data->live.blas_builds > builds_before, "event counts still advance while off (builds %llu)", (unsigned long long)data->live.blas_builds);
  CHECK(PerfSumsZero(data), "gathering off: no sum or last value is written");
  for (size_t k = 0; k < bvh::kPerfBlockCount; ++k) {
    CHECK(bvh::g_perf_gate.block_us[k].load() == 0u, "gathering off: block %zu adds nothing", k);
  }
  CHECK(bvh::g_pool.cpu_hook_frame_us.load() == 777u && bvh::g_pool.cpu_drain_frame_us.load() == 777u
            && bvh::g_pool.cpu_capture_frame_us.load() == 777u && bvh::g_deform_live.cpu_frame_us.load() == 777u,
        "gathering off: frame values are not exchanged");
  CHECK(bvh::g_perf_gate.present_world_us.load() == 0u && bvh::g_perf_gate.present_bvh_us.load() == 0u
            && bvh::g_perf_gate.alpha_sync_us.load() == 0u && bvh::g_perf_gate.rtao_record_us.load() == 0u
            && bvh::g_perf_gate.deform_update_us.load() == 0u,
        "gathering off: no bracket or stopwatch adds");
  {
    std::lock_guard<std::mutex> lock(bvh::g_perf.mutex);
    CHECK(bvh::g_perf.tab.count == 0u && !bvh::g_perf.tab.primed, "gathering off: the tab takes no sample");
  }
  CHECK(std::isnan(PerfTabNow().max.frame) && std::isnan(PerfTabNow().max.cpu) && std::isnan(PerfTabNow().max.row[bvh::kPerfTlasRebuild]),
        "gathering off: the tab maxima are --");
  CHECK(!PerfTabNow().spike.valid, "gathering off: the tab has no spike");
  {
    std::lock_guard<std::mutex> lock(bvh::g_cache_diag.mutex);
    CHECK(bvh::g_cache_diag.data.frame_intervals == 0u, "gathering off: no frame interval");
  }
  bvh::g_perf_gate.gather.store(true);
  ResetPerfTab();
  AddMesh(0x701u, BoxMesh(0.9f));
  AddInstance(0x701u, 130.f, 0.f, 130.f);
  for (int i = 0; i < 12; ++i) {  // a TLAS rebuild comes every kLiveTlasInterval (8) presents
    falcom_world::g_state.frame.fetch_add(1u);
    bvh::SamplePerfPresent(dev);
    bvh::UpdateLiveBvh(dev, queue);
    bvh::UpdateCacheDiag(dev);
  }
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    CHECK(bvh::g_pool.tlas_timing.rebuild_ms_sum > 0.f && data->live.blas_build_ms_sum > 0.f && bvh::g_pool.tlas_timing.rebuild_phase_ms_sum[2] > 0.f,
          "gathering on: rebuild, BLAS and phase sums are written (%.4f, %.4f)", bvh::g_pool.tlas_timing.rebuild_ms_sum, data->live.blas_build_ms_sum);
    CHECK(bvh::g_perf_gate.block_us[bvh::kPerfBlockPerfSample].load() > 0u && bvh::g_perf_gate.block_us[bvh::kPerfBlockCacheDiag].load() > 0u
              && bvh::g_perf_gate.block_us[bvh::kPerfBlockRebuildCall].load() > 0u,
          "gathering on: perf_sample, cache_diag and rebuild_call blocks are timed");
  }
  {
    std::lock_guard<std::mutex> lock(bvh::g_perf.mutex);
    CHECK(bvh::g_perf.tab.count == 11u, "gathering on: the first present primes, the next eleven feed the tab (count %zu)", bvh::g_perf.tab.count);
  }
  // The switch goes OFF: the clear zeroes what was gathered, and the dump says so.
  bvh::g_perf_gate.gather.store(false);
  bvh::ClearPerfGathered(bvh::GetBvhDeviceData(dev));
  CHECK(PerfSumsZero(data), "the clear on switch-off zeroes the sums and last values");
  {
    std::lock_guard<std::mutex> lock(bvh::g_perf.mutex);
    CHECK(bvh::g_perf.tab.count == 0u, "the clear empties the tab window");
  }
  const std::string text = ReadDumpText(bvh::WriteCacheDiagFile(data, 1u, "", bvh::CacheSwitches{}));
  CHECK(JsonError(text).empty(), "cache dump after off is valid JSON: %s", JsonError(text).c_str());
  CHECK(text.find("\"perf_gather\": false") != std::string::npos, "the dump after off says perf_gather false");
  // A run aborted by switching OFF: five presents are in the series and the OFF present adds nothing to it or to the spike.
  bvh::g_perf_gate.gather.store(true);
  ResetPerfTab();
  bvh::ResetCacheDiagCounters();
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    bvh::g_pool.episode_start_frame = 0u;
  }
  const auto base = std::chrono::steady_clock::now();
  const bvh::CacheSwitches sw{true, false, false, true};
  bvh::RequestPerfRun("off");
  bvh::UpdatePerfRun(data, 9000u, base, sw);
  for (int i = 1; i <= 5; ++i) bvh::UpdatePerfRun(data, 9000u + static_cast<uint32_t>(i), base + std::chrono::milliseconds(16 * i), sw);
  bvh::g_perf_gate.gather.store(false);
  bvh::UpdatePerfRun(data, 9006u, base + std::chrono::milliseconds(16 * 6 + 500), sw);
  const std::string off_run = ReadDumpText(WrittenPath(PerfResult()));
  CHECK(JsonError(off_run).empty(), "aborted run file is valid JSON: %s", JsonError(off_run).c_str());
  CHECK(off_run.find("\"status\": \"aborted\"") != std::string::npos && off_run.find("\"length\": 5,") != std::string::npos,
        "switching off aborts the run with five presents in the series");
  CHECK(ValueAt(off_run, "\"worst_present\": {", "\"index\"") < 5.0, "no spike from the OFF present");
  bvh::g_perf_gate.gather.store(true);
  ResetPerfTab();
  bvh::ResetCacheDiagCounters();
}

// Dumps and the diagnostics header say whether the timings were gathered; the timing keys added to the cache dump.
static void TestPerfJsonKeys(Device* dev) {
  bvh::ResetCacheDiagCounters();
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(dev);
  data->live.mesh_ms_last = 1.5f;
  data->live.mesh_ms_max = 2.5f;
  data->live.tlas_skips = 3u;
  data->live.tlas_skip_ms_sum = 0.75f;
  data->live.tlas_skip_ms_max = 0.5f;
  {
    std::lock_guard<std::mutex> lock(bvh::g_cache_diag.mutex);
    bvh::g_cache_diag.data.frame_ms_last = 20.f;
    bvh::g_cache_diag.data.frame_ms_sum = 60.0;
    bvh::g_cache_diag.data.frame_intervals = 3u;
  }
  bvh::g_perf_gate.gather.store(true);
  std::string text = ReadDumpText(bvh::WriteCacheDiagFile(data, 1u, "", bvh::CacheSwitches{}));
  CHECK(JsonError(text).empty(), "cache dump is valid JSON: %s", JsonError(text).c_str());
  CHECK(text.find("\"perf_gather\": true") != std::string::npos, "dump with gathering on says perf_gather true");
  CHECK(text.find("\"mesh_ms\": {\"last\": 1.5, \"max\": 2.5}") != std::string::npos, "mesh_ms last and max (%s)", "live block");
  CHECK(text.find("\"tlas_skips\": 3, \"skip_ms\": {\"sum\": 0.75, \"max\": 0.5}") != std::string::npos, "skip_ms sum and max");
  CHECK(text.find("\"frame_ms\": {\"last\": 20, \"avg\": 20, \"intervals\": 3}") != std::string::npos, "frame_ms avg is sum over intervals");
  {
    std::lock_guard<std::mutex> lock(bvh::g_cache_diag.mutex);
    bvh::g_cache_diag.data.frame_intervals = 0u;
  }
  text = ReadDumpText(bvh::WriteCacheDiagFile(data, 1u, "", bvh::CacheSwitches{}));
  CHECK(text.find("\"frame_ms\": {\"last\": 20, \"avg\": -1, \"intervals\": 0}") != std::string::npos, "frame_ms avg is -1 with no interval");
  bvh::g_perf_gate.gather.store(false);
  CHECK(bvh::CacheDiagLines(data)[0].find("(performance metrics OFF: timings read 0)") != std::string::npos, "header line says OFF");
  text = ReadDumpText(bvh::WriteCacheDiagFile(data, 1u, "", bvh::CacheSwitches{}));
  CHECK(text.find("\"perf_gather\": false") != std::string::npos, "dump with gathering off says perf_gather false");
  bvh::g_perf_gate.gather.store(true);
  data->live.mesh_ms_last = 0.f;
  data->live.mesh_ms_max = 0.f;
  data->live.tlas_skips = 0u;
  data->live.tlas_skip_ms_sum = 0.f;
  data->live.tlas_skip_ms_max = 0.f;
  const std::vector<std::string> lines = bvh::CacheDiagLines(data);
  CHECK(lines[0].find("OFF") == std::string::npos, "header line has no OFF suffix with gathering on");
  // The perf file keys added for the maxima, the hitches, the series and the dynamic gauge (an empty run: the values are null).
  bvh::PerfRunState empty;
  const std::string perf = ReadDumpText(bvh::WritePerfFile(empty));
  CHECK(JsonError(perf).empty(), "empty perf file is valid JSON: %s", JsonError(perf).c_str());
  for (const char* key : {"\"sample_alignment\"", "\"max_per_present\"", "\"max_per_measured_frame\"", "\"p99\"", "\"hitches_over_25ms\"", "\"hitches_over_33_3ms\"",
                          "\"series\"", "\"tlas_dynamic_instances\"", "\"attribution\"", "\"presents_over_2ms\"", "\"threshold_ms\"", "\"schema\": 1",
                          "\"worst_present\"", "\"worst_unattributed_present\"", "\"settings_cost\"", "\"inclusion_switches\"", "\"blocks_unit\""}) {
    CHECK(perf.find(key) != std::string::npos, "perf file has the key %s", key);
  }
  bvh::ResetCacheDiagCounters();
}

// The perf file keeps the RTAO and alpha timers when they are passed in (UpdatePerfRun extra).
static void TestPerfExtraGpu(Device* dev) {
  bvh::ResetCacheDiagCounters();
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(dev);
  uint32_t saved_start = 0u;
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    saved_start = bvh::g_pool.episode_start_frame;
    bvh::g_pool.episode_start_frame = 0u;
  }
  const auto base = std::chrono::steady_clock::now();
  const bvh::CacheSwitches sw{true, false, false, true};
  bvh::g_cache_diag.enabled.store(true);
  bvh::RequestPerfRun("extra");
  bvh::PerfExtraGpu extra;
  bvh::UpdatePerfRun(data, 9000u, base, sw, extra);
  int k = 0;
  while (bvh::g_perf.active.load() && k < 5000) {
    ++k;
    PerfPresentCosts(data);
    extra.rtao.ms_sum += 1.0;
    extra.rtao.samples += 1u;
    data->alpha.timer.ms_sum += 0.5;
    data->alpha.timer.samples += 1u;
    bvh::UpdatePerfRun(data, 9000u + static_cast<uint32_t>(k), base + std::chrono::milliseconds(16 * k), sw, extra);
  }
  const std::string text = ReadDumpText(WrittenPath(PerfResult()));
  {
    std::lock_guard<std::mutex> lock(bvh::g_pool.mutex);
    bvh::g_pool.episode_start_frame = saved_start;
  }
  CHECK(JsonError(text).empty(), "perf file with extras is valid JSON: %s", JsonError(text).c_str());
  CHECK(text.find("\"gpu_rtao\": {\"basis\": \"measured\", \"avg_per_measured_frame\": {\"value\": 1,") != std::string::npos, "RTAO timer averages 1 ms per measured frame");
  CHECK(text.find("\"gpu_alpha_blit\": {\"basis\": \"measured\", \"avg_per_measured_frame\": {\"value\": 0.5,") != std::string::npos, "alpha blit timer averages 0.5 ms");
  CHECK(text.find("\"gpu_rtao_filter\": {\"basis\": \"not measured\"") != std::string::npos, "RTAO filter without samples is not measured");
  CHECK(text.find("\"gpu_rtls\": {\"basis\": \"not measured\"") != std::string::npos, "RTLS trace without samples is not measured");
  CHECK(text.find("\"gpu_rtls_denoise\": {\"basis\": \"not measured\"") != std::string::npos, "RTLS denoise without samples is not measured");
  CHECK(text.find("\"rtls_record_cpu\"") != std::string::npos, "perf file has rtls_record_cpu");
  CHECK(CountOf(text, "\"value\": ") == CountOf(text, "\"unit\": "), "every value object has a unit (extras)");
  bvh::ResetCacheDiagCounters();
}

// M3 (rt12): the store hooks feed the D3D12 mirror only while it is active; the helpers follow their rules; the
// material copy keeps its version with the mirror off.
static void TestRt12Mirror(Device* dev, Queue* queue) {
  namespace rt12 = falcom_world::rt12;
  Stage("rt12 mirror");
  // 1. Indices: out-of-range or non-finite triangles become 0,0,0; the trailing partial triangle stays.
  {
    const float v[16] = {0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, NAN, 0.f, 0.f, 1.f};
    uint32_t idx[13] = {0u, 1u, 2u, 0u, 1u, 9u, 0u, 3u, 1u, 2u, 1u, 0u, 5u};
    const uint32_t want[13] = {0u, 1u, 2u, 0u, 0u, 0u, 0u, 0u, 0u, 2u, 1u, 0u, 5u};
    const uint32_t changed = rt12::SanitizeRt12Indices(v, 4u, idx, 13u);
    CHECK(changed == 2u, "sanitize changed %u triangles", changed);
    for (int i = 0; i < 13; ++i) CHECK(idx[i] == want[i], "sanitize index %d is %u", i, idx[i]);
  }
  // 2. Instance fields: visibility flags to mask, material flag to opaque, world floats to transform.
  {
    const float world[16] = {1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f};
    CHECK(rt12::MakeRt12InstanceFields(world, 2.f, 0.f).mask == 1u, "camera visible: mask 1");
    CHECK(rt12::MakeRt12InstanceFields(world, 10.f, 0.f).mask == 2u, "camera visible foliage: mask 2");
    CHECK(rt12::MakeRt12InstanceFields(world, 18.f, 0.f).mask == 2u, "camera visible alpha-tested: mask 2");
    CHECK(rt12::MakeRt12InstanceFields(world, 4.f, 0.f).mask == 4u, "shadow only: mask 4");
    CHECK(rt12::MakeRt12InstanceFields(world, 0.f, 0.f).mask == 4u, "not visible: mask 4");
    CHECK(rt12::MakeRt12InstanceFields(world, 2.f, 0.f).opaque, "material 0: opaque");
    CHECK(!rt12::MakeRt12InstanceFields(world, 2.f, 3.f).opaque, "material 3: not opaque");
    const rt12::Rt12InstanceFields fields = rt12::MakeRt12InstanceFields(world, 2.f, 0.f);
    bool same = true;
    for (int i = 0; i < 12; ++i) same = same && fields.transform[i] == world[i];
    CHECK(same, "transform rows are the world floats");
  }
  // 3. Mirror through the live store: the uploads queue the arena appends and the new slots at the D3D11 offsets.
  {
    const auto reset_mirror = [](bool active) {
      std::lock_guard lock(rt12::g_rt12_mirror.mutex);
      rt12::g_rt12_mirror.active = active;
      rt12::g_rt12_mirror.reset = false;
      rt12::g_rt12_mirror.appends.clear();
      rt12::g_rt12_mirror.new_slots.clear();
      rt12::g_rt12_mirror.retired_slots.clear();
    };
    bvh::ResetWorldPool();
    auto& frame = falcom_world::g_state.frame;
    frame.store(6000u);
    SetCamera(10.f, 5.f, 10.f);
    bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(dev);
    const auto present = [&]() {
      frame.fetch_add(1u);
      bvh::SamplePerfPresent(dev);
      bvh::UpdateLiveBvh(dev, queue);
    };
    bvh::PoolDecodedMesh uv_mesh = BoxMesh(1.f);
    for (const auto& p : uv_mesh.positions) uv_mesh.uvs.push_back({p[1] * 0.5f + 0.5f, p[2] * 0.5f + 0.5f});
    AddMesh(0x71, uv_mesh);
    AddMesh(0x72, Grid(4, 4, 4.f, 0.5f));
    AddInstance(0x71, 0.f, 0.f, 20.f);
    AddInstance(0x72, 6.f, 0.f, 20.f);

    reset_mirror(true);
    const uint64_t vertices_before = data->vertices.used;
    const uint64_t indices_before = data->indices.used;
    const uint64_t uvs_before = data->uvs.used;
    for (int i = 0; i < 30; ++i) present();
    CHECK(data->slots.size() == 2u, "two live slots (%zu)", data->slots.size());
    {
      std::lock_guard lock(rt12::g_rt12_mirror.mutex);
      uint64_t bytes[3] = {0u, 0u, 0u};
      bool first_seen[3] = {false, false, false};
      bool first_at_before = true;
      for (const rt12::Rt12MirrorAppend& append : rt12::g_rt12_mirror.appends) {
        bytes[append.arena] += append.bytes.size();
        const uint64_t before = append.arena == rt12::kRt12ArenaVertices ? vertices_before
                              : append.arena == rt12::kRt12ArenaIndices ? indices_before : uvs_before;
        if (!first_seen[append.arena]) first_at_before = first_at_before && append.first_element == before;
        first_seen[append.arena] = true;
      }
      CHECK(first_at_before, "each arena's first append starts at the old element count");
      CHECK(bytes[rt12::kRt12ArenaVertices] == (data->vertices.used - vertices_before) * 16u, "vertex bytes queued (%llu)",
            (unsigned long long)bytes[rt12::kRt12ArenaVertices]);
      CHECK(bytes[rt12::kRt12ArenaIndices] == (data->indices.used - indices_before) * 4u, "index bytes queued (%llu)",
            (unsigned long long)bytes[rt12::kRt12ArenaIndices]);
      CHECK(bytes[rt12::kRt12ArenaUvs] == (data->uvs.used - uvs_before) * 8u, "UV bytes queued (%llu)",
            (unsigned long long)bytes[rt12::kRt12ArenaUvs]);
      CHECK(rt12::g_rt12_mirror.new_slots.size() == data->slots.size(), "new slots queued (%zu)", rt12::g_rt12_mirror.new_slots.size());
      for (const rt12::Rt12MirrorSlot& queued : rt12::g_rt12_mirror.new_slots) {
        const bvh::LiveMeshSlot& live = data->slots[queued.slot];
        CHECK(live.vertex_offset == queued.vertex_offset && live.vertex_count == queued.vertex_count
                  && live.index_offset == queued.index_offset && live.index_count == queued.index_count,
              "queued slot %u matches the store", queued.slot);
      }
      CHECK(rt12::g_rt12_mirror.sanitized_triangles == 0u, "no invalid triangles in the test meshes");
    }
    // Retire one slot: its index is queued.
    const uint64_t kept_uid = data->slots[0].uid;
    CHECK(bvh::RetireLiveSlots(data, {kept_uid}) == 1u, "one slot retired");
    {
      std::lock_guard lock(rt12::g_rt12_mirror.mutex);
      CHECK(rt12::g_rt12_mirror.retired_slots.size() == 1u && rt12::g_rt12_mirror.retired_slots[0] == 1u,
            "retired slot queued (%zu)", rt12::g_rt12_mirror.retired_slots.size());
    }
    // A store reset drops everything queued before it.
    bvh::DestroyLiveStore(dev, data);
    {
      std::lock_guard lock(rt12::g_rt12_mirror.mutex);
      CHECK(rt12::g_rt12_mirror.reset && rt12::g_rt12_mirror.appends.empty() && rt12::g_rt12_mirror.new_slots.empty()
                && rt12::g_rt12_mirror.retired_slots.empty(),
            "store reset: reset flag set, queues empty");
    }
    // 4. Mirror inactive: the same uploads queue nothing.
    reset_mirror(false);
    bvh::ResetWorldPool();
    AddMesh(0x73, uv_mesh);
    AddInstance(0x73, 0.f, 0.f, 20.f);
    for (int i = 0; i < 30; ++i) present();
    {
      std::lock_guard lock(rt12::g_rt12_mirror.mutex);
      CHECK(rt12::g_rt12_mirror.appends.empty() && rt12::g_rt12_mirror.new_slots.empty() && !rt12::g_rt12_mirror.reset,
            "inactive mirror queues nothing");
    }
    bvh::ResetWorldPool();
    present();
  }
  // 5. Materials with the mirror inactive: one slice, the out-of-range slice ignored, cleared, kept across a store reset.
  {
    uint8_t bytes[32];
    std::memset(bytes, 0x5A, sizeof(bytes));
    uint64_t version = 0u;
    {
      std::lock_guard lock(rt12::g_rt12_mirror.mutex);
      version = rt12::g_rt12_mirror.materials_version;
    }
    rt12::MirrorMaterial(3u, bytes);
    {
      std::lock_guard lock(rt12::g_rt12_mirror.mutex);
      CHECK(rt12::g_rt12_mirror.materials[3][0] == 0x5Au && rt12::g_rt12_mirror.materials[3][31] == 0x5Au, "slice 3 copied");
      CHECK(rt12::g_rt12_mirror.materials_version == version + 1u, "slice 3 moves the version");
    }
    rt12::MirrorMaterial(256u, bytes);
    {
      std::lock_guard lock(rt12::g_rt12_mirror.mutex);
      CHECK(rt12::g_rt12_mirror.materials_version == version + 1u, "slice 256 ignored");
    }
    rt12::MirrorMaterial(5u, bytes);
    bvh::DestroyLiveStore(dev, bvh::GetBvhDeviceData(dev));
    {
      std::lock_guard lock(rt12::g_rt12_mirror.mutex);
      CHECK(rt12::g_rt12_mirror.materials[5][0] == 0x5Au, "store reset keeps the materials");
    }
    rt12::MirrorMaterialsCleared();
    {
      std::lock_guard lock(rt12::g_rt12_mirror.mutex);
      bool zero = true;
      for (const auto& slice : rt12::g_rt12_mirror.materials) for (uint8_t b : slice) zero = zero && b == 0u;
      CHECK(zero, "cleared materials are zero");
      CHECK(rt12::g_rt12_mirror.materials_version == version + 3u, "clear moves the version");
    }
  }
  {
    std::lock_guard lock(rt12::g_rt12_mirror.mutex);
    rt12::g_rt12_mirror.active = false;
    rt12::g_rt12_mirror.reset = false;
    rt12::g_rt12_mirror.appends.clear();
    rt12::g_rt12_mirror.new_slots.clear();
    rt12::g_rt12_mirror.retired_slots.clear();
  }
}

// M4 (rt12): the character vertex range, the BLAS build cadence and the carrier capacity.
static void TestRt12Dynamic() {
  namespace rt12 = falcom_world::rt12;
  Stage("rt12 dynamic");
  // 1. Vertex end: the largest vertex_base + 3 * triangles over the characters; 0 for none.
  {
    const uint32_t bases[3] = {0u, 300u, 90u};
    const uint32_t triangles[3] = {10u, 5u, 100u};
    CHECK(rt12::Rt12DynamicVertexEnd(bases, triangles, 3u) == 390u, "vertex end of three characters is 390");
    CHECK(rt12::Rt12DynamicVertexEnd(bases, triangles, 0u) == 0u, "no characters: vertex end 0");
  }
  // 2. BLAS cadence: built when new, when the triangle count moved, and every 30 frames; otherwise refit.
  CHECK(rt12::Rt12DynamicNeedsBuild(false, 0u, 10u, 0u), "a new identity builds");
  CHECK(rt12::Rt12DynamicNeedsBuild(true, 10u, 11u, 1u), "a changed triangle count builds");
  CHECK(!rt12::Rt12DynamicNeedsBuild(true, 10u, 10u, 29u), "29 frames after the build refits");
  CHECK(rt12::Rt12DynamicNeedsBuild(true, 10u, 10u, 30u), "30 frames after the build rebuilds");
  // 3. The carrier holds the whole deform arena (float3 vertices, 12 bytes each).
  CHECK(static_cast<uint64_t>(rt12::kRt12CarrierWidth) * rt12::kRt12CarrierHeight * 12u >= bvh::kDynamicArenaBytes,
        "carrier holds the deform arena");
}

// M5 (rt12): a P2b record's D3D12 instance desc (rt12::Rt12RecordDescWords, the transcription of world_rt12_records.cs_6_5.hlsl)
// equals the desc the static path builds (MakeRt12InstanceFields); an invalid record or one without a BLAS is inactive.
static void TestRt12Records() {
  namespace rt12 = falcom_world::rt12;
  Stage("rt12 records");
  const uint32_t kStatic = 7u;  // S: the static instance count (InstanceID of record r = S + r)
  const uint64_t kAddress = 0x12340000ull;
  // 1. Four records: camera-visible valid, shadow-only valid, invalid (header.w 0) with an address, valid without a BLAS.
  struct Case {
    float header_w;
    float visibility_z;
    uint64_t address;
    uint32_t mask;
    uint64_t blas;
  };
  const Case cases[4] = {{1.f, 2.f, kAddress, 1u, kAddress}, {1.f, 4.f, kAddress, 4u, kAddress}, {0.f, 2.f, kAddress, 0u, 0u},
                         {1.f, 2.f, 0u, 0u, 0u}};
  for (uint32_t r = 0u; r < 4u; ++r) {
    float record[48] = {};
    record[0] = static_cast<float>(r);  // header.x: live store slot
    record[2] = 0.f;                    // header.z: opaque material
    record[3] = cases[r].header_w;      // header.w: valid
    for (uint32_t k = 0u; k < 12u; ++k) record[4u + k] = 0.1f * static_cast<float>(k + 1u) + static_cast<float>(r);
    record[46] = cases[r].visibility_z;  // visibility.z: INSTANCE_* flags
    uint32_t words[16] = {};
    rt12::Rt12RecordDescWords(record, kStatic + r, cases[r].address, words);
    D3D12_RAYTRACING_INSTANCE_DESC desc = {};
    std::memcpy(&desc, words, sizeof(desc));
    CHECK(desc.InstanceID == kStatic + r, "record %u: InstanceID is S + r", r);
    CHECK(desc.InstanceMask == cases[r].mask, "record %u: mask %u", r, cases[r].mask);
    CHECK(desc.AccelerationStructure == cases[r].blas, "record %u: BLAS address", r);
    // The static path's desc for the same record (MakeRt12InstanceFields); mask and address as the rules above.
    const rt12::Rt12InstanceFields fields = rt12::MakeRt12InstanceFields(record + 4, record[46], record[2]);
    D3D12_RAYTRACING_INSTANCE_DESC expect = {};
    std::memcpy(expect.Transform, fields.transform, sizeof(expect.Transform));
    expect.InstanceID = kStatic + r;
    expect.InstanceMask = cases[r].mask;
    expect.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE
                   | (fields.opaque ? D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE : D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE);
    expect.AccelerationStructure = cases[r].blas;
    CHECK(sizeof(desc) == 64u, "the D3D12 instance desc is 64 bytes");
    CHECK(std::memcmp(&desc, &expect, sizeof(desc)) == 0, "record %u: the 64 bytes equal the static path's desc", r);
  }
  // 3. GPU-cache records (G4): table base 0. A is valid, camera-visible, non-opaque (header.z 3); B is invalid (header.w 0).
  {
    float record[2][48] = {};
    record[0][0] = 5.f;
    record[0][2] = 3.f;
    record[0][3] = 1.f;
    record[0][46] = 2.f;
    record[1][0] = 6.f;
    D3D12_RAYTRACING_INSTANCE_DESC descs[2] = {};
    for (uint32_t r = 0u; r < 2u; ++r) {
      for (uint32_t k = 0u; k < 12u; ++k) record[r][4u + k] = 0.1f * static_cast<float>(k + 1u);
      uint32_t words[16] = {};
      rt12::Rt12RecordDescWords(record[r], r, kAddress, words);
      std::memcpy(&descs[r], words, sizeof(descs[r]));
    }
    CHECK(descs[0].InstanceID == 0u && descs[0].InstanceMask == 1u && descs[0].AccelerationStructure == kAddress,
          "GPU-cache record A: InstanceID 0, mask 1, BLAS address");
    CHECK((static_cast<uint32_t>(descs[0].Flags) & static_cast<uint32_t>(D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE)) != 0u,
          "GPU-cache record A: FORCE_NON_OPAQUE (header.z 3)");
    CHECK(descs[1].InstanceMask == 0u && descs[1].AccelerationStructure == 0u, "GPU-cache record B (invalid): mask 0, BLAS 0");
  }
  CHECK(static_cast<uint64_t>(rt12::kRt12CarrierWidth) * rt12::kRt12GcRecordCarrierHeight
            >= static_cast<uint64_t>(bvh::kGcSetCapacity + bvh::kGcMovingRecords) * rt12::kRt12RecordTexels,
        "GPU-cache records carrier holds set + moving records");
  // 2. The records carrier holds every record: 12 texels each, 32768 records.
  CHECK(static_cast<uint64_t>(rt12::kRt12CarrierWidth) * rt12::kRt12RecordCarrierHeight
            >= static_cast<uint64_t>(bvh::kPoolDynamicInstances) * rt12::kRt12RecordTexels,
        "records carrier holds kPoolDynamicInstances records");
}

// ---------------------------------------------------------------------------
// G2 GPU geometry cache: the transcriptions (decode, carrier layout, budget, compare) and the draw-side hook.
// ---------------------------------------------------------------------------

// The decode passes against the CPU decode, bit for bit: every position format x index size x UV layout, with an
// unaligned position (stride 14, offset 6, for the half formats), base_vertex -3, odd first_index, and half subnormals.
static void TestGcDecode() {
  namespace gc = falcom_world::bvh;
  namespace scene = renodx::utils::scene;
  Stage("gpu capture decode");
  struct PosCase {
    uint32_t code;
    reshade::api::format format;
    uint32_t bytes;
    uint32_t offset;
  };
  struct UvCase {
    uint32_t code;
    reshade::api::format format;
    uint32_t bytes;
  };
  const PosCase poses[5] = {{gc::kGcFormatF32x2, reshade::api::format::r32g32_float, 8u, 0u},
                            {gc::kGcFormatF32x3, reshade::api::format::r32g32b32_float, 12u, 0u},
                            {gc::kGcFormatF32x4, reshade::api::format::r32g32b32a32_float, 16u, 0u},
                            {gc::kGcFormatF16x2, reshade::api::format::r16g16_float, 4u, 6u},
                            {gc::kGcFormatF16x4, reshade::api::format::r16g16b16a16_float, 8u, 6u}};
  const UvCase uvs[3] = {{gc::kGcFormatNone, reshade::api::format::unknown, 0u},
                         {gc::kGcFormatF16x2, reshade::api::format::r16g16_float, 4u},
                         {gc::kGcFormatF32x2, reshade::api::format::r32g32_float, 8u}};
  const uint32_t kVertices = 50u, kTriangles = 40u, kFirstIndex = 3u;
  std::mt19937 rng(7u);
  for (const PosCase& pos : poses) {
    for (const uint32_t index_size : {2u, 4u}) {
      for (const UvCase& uv : uvs) {
        const bool has_uv = uv.code != gc::kGcFormatNone;
        const bool half = pos.format == reshade::api::format::r16g16_float || pos.format == reshade::api::format::r16g16b16a16_float;
        const uint32_t stride = pos.offset + pos.bytes + uv.bytes + 2u;
        const uint32_t uv_offset = pos.offset + pos.bytes;
        const uint32_t index_count = 3u * kTriangles;
        // Vertex bytes: random finite values; vertex 1 holds the half subnormal 0x0200 in x.
        std::vector<uint8_t> vb(kVertices * stride, 0u);
        const auto fill = [&](uint8_t* dst, bool is_half, uint32_t bytes) {
          const uint32_t count = is_half ? bytes / 2u : bytes / 4u;
          for (uint32_t c = 0u; c < count; ++c) {
            if (is_half) {
              uint16_t bits = static_cast<uint16_t>(rng() & 0xFFFFu);
              if (((bits >> 10u) & 0x1Fu) == 31u) bits = static_cast<uint16_t>(bits & ~0x0400u);
              std::memcpy(dst + 2u * c, &bits, 2u);
            } else {
              const float value = std::uniform_real_distribution<float>(-100.f, 100.f)(rng);
              std::memcpy(dst + 4u * c, &value, 4u);
            }
          }
        };
        for (uint32_t v = 0u; v < kVertices; ++v) {
          fill(vb.data() + v * stride + pos.offset, half, pos.bytes);
          if (has_uv) fill(vb.data() + v * stride + uv_offset, uv.code == gc::kGcFormatF16x2, uv.bytes);
        }
        const uint16_t subnormal = 0x0200u;
        std::memcpy(vb.data() + stride + pos.offset, &subnormal, 2u);  // half formats read it as a half
        // Index bytes: the draw reads from first_index; the three before it are garbage.
        std::vector<uint8_t> ib((kFirstIndex + index_count) * index_size, 0xEEu);
        for (uint32_t i = 0u; i < index_count; ++i) {
          const uint32_t raw = 3u + (rng() % kVertices);  // absolute = raw - 3 in [0, kVertices)
          const uint32_t at = (kFirstIndex + i) * index_size;
          if (index_size == 2u) {
            const uint16_t value = static_cast<uint16_t>(raw);
            std::memcpy(ib.data() + at, &value, 2u);
          } else {
            std::memcpy(ib.data() + at, &raw, 4u);
          }
        }
        const int32_t base_vertex = -3;
        const uint64_t index_begin = uint64_t{kFirstIndex} * index_size;
        const uint64_t index_bytes = uint64_t{index_count} * index_size;

        // CPU reference.
        std::vector<uint32_t> cpu_indices;
        uint32_t min_vertex = 0u, max_vertex = 0u, raw_min = 0u, raw_max = 0u;
        uint32_t first_raw[gc::kPoolFirstRawCount] = {};
        const char* index_error = gc::DecodePoolMeshIndices(ib.data() + index_begin, index_bytes, index_size, index_count, base_vertex,
                                                             stride, &cpu_indices, &min_vertex, &max_vertex, &raw_min, &raw_max, first_raw);
        CHECK(index_error == nullptr, "cpu index decode: %s", index_error ? index_error : "");
        const uint64_t vertex_bytes = uint64_t{max_vertex - min_vertex + 1u} * stride;
        gc::PoolDecodedMesh cpu_mesh;
        const char* vertex_error = gc::DecodePoolMeshVertices(vb.data() + uint64_t{min_vertex} * stride, vertex_bytes, stride,
                                                              static_cast<int32_t>(pos.offset), pos.format,
                                                              has_uv ? static_cast<int32_t>(uv_offset) : -1, uv.format, cpu_indices,
                                                              min_vertex, max_vertex, &cpu_mesh);
        CHECK(vertex_error == nullptr, "cpu vertex decode: %s", vertex_error ? vertex_error : "");

        // GPU side: the copies placed as the hook places them (16-aligned offsets, 4-aligned copies).
        std::vector<uint8_t> staging(64u * 1024u, 0u);
        uint64_t cursor = 0u;
        const auto place = [&](const std::vector<uint8_t>& source, uint64_t begin, uint64_t end) {
          const uint64_t offset = (cursor + 15u) & ~uint64_t{15u};
          const uint64_t aligned_begin = begin & ~uint64_t{3u};
          const uint64_t copied = (std::min)((end + 3u) & ~uint64_t{3u}, uint64_t{source.size()}) - aligned_begin;
          std::memcpy(staging.data() + offset, source.data() + aligned_begin, copied);
          cursor = offset + copied;
          return static_cast<uint32_t>(offset + (begin - aligned_begin));
        };
        const uint32_t index_byte = place(ib, index_begin, index_begin + index_bytes);
        const uint32_t vertex_byte = place(vb, uint64_t{min_vertex} * stride, uint64_t{max_vertex + 1u} * stride);
        std::array<uint32_t, 4> range = {};
        gc::GcRangeJob(staging.data(), staging.size(), index_byte, index_size, index_count, base_vertex, range.data());
        CHECK(~range[0] == min_vertex && range[1] == max_vertex && range[2] == 0u, "range job: min %u max %u invalid %u (cpu %u..%u)",
              ~range[0], range[1], range[2], min_vertex, max_vertex);

        const uint32_t vertices = max_vertex - min_vertex + 1u;
        const uint32_t texels = gc::GcJobTexels(vertices, index_count, has_uv);
        gc::GcDecodeParams params;
        params.index_byte = index_byte;
        params.index_size = index_size;
        params.index_count = index_count;
        params.base_vertex = base_vertex;
        params.vertex_byte = vertex_byte;
        params.stride = stride;
        params.pos_offset = pos.offset;
        params.pos_format = pos.code;
        params.uv_byte = vertex_byte;
        params.uv_stride = stride;
        params.uv_offset = has_uv ? uv_offset : gc::kGcNoUv;
        params.uv_format = uv.code;
        params.vertex_count = vertices;
        params.min_vertex = min_vertex;
        params.vtx_texel = 0u;
        params.idx_texel = vertices;
        std::vector<uint32_t> carrier(size_t{texels} * 4u, 0u);
        gc::GcDecodeJob(staging.data(), staging.size(), params, carrier.data());

        std::vector<float> gpu_vertices(size_t{vertices} * 4u);
        std::vector<uint32_t> gpu_indices(index_count);
        std::vector<float> gpu_uvs(size_t{vertices} * 2u);
        std::memcpy(gpu_vertices.data(), carrier.data(), gpu_vertices.size() * sizeof(float));
        for (uint32_t i = 0u; i < index_count; ++i) gpu_indices[i] = carrier[(size_t{vertices + i / 4u}) * 4u + i % 4u];
        const uint32_t uv_texel = vertices + (index_count + 3u) / 4u;
        for (uint32_t v = 0u; has_uv && v < vertices; ++v) {
          std::memcpy(gpu_uvs.data() + 2u * v, carrier.data() + (size_t{uv_texel + v / 2u} * 4u + (v % 2u) * 2u), 2u * sizeof(float));
        }
        std::vector<uint32_t> cpu_flat;
        for (const auto& triangle : cpu_mesh.triangles) cpu_flat.insert(cpu_flat.end(), triangle.begin(), triangle.end());
        const gc::GcCompareResult compare = gc::GcCompareTriangles(gpu_vertices.data(), vertices, gpu_indices.data(), index_count,
                                                                  has_uv ? gpu_uvs.data() : nullptr, cpu_mesh.positions, cpu_flat,
                                                                  cpu_mesh.uvs);
        CHECK(compare.mismatched == 0u && compare.first < 0, "pos %u index %u uv %u: %u mismatched triangles (first %lld)", pos.code,
              index_size, uv.code, compare.mismatched, static_cast<long long>(compare.first));
        CHECK(compare.triangles == kTriangles, "triangles %u, expected %u", compare.triangles, kTriangles);
        CHECK(compare.uv_compared == (has_uv && !cpu_mesh.uvs.empty()), "uv compared flag %d", compare.uv_compared ? 1 : 0);
      }
    }
  }

  // HalfToFloat: the transcription is bit-exact for all 65536 inputs (including the CPU's subnormal branch).
  uint32_t half_mismatches = 0u;
  for (uint32_t bits = 0u; bits < 65536u; ++bits) {
    const float ours = gc::GcHalfToFloat(bits);
    const float theirs = scene::HalfToFloat(static_cast<uint16_t>(bits));
    uint32_t ours_bits = 0u, theirs_bits = 0u;
    std::memcpy(&ours_bits, &ours, sizeof(ours_bits));
    std::memcpy(&theirs_bits, &theirs, sizeof(theirs_bits));
    if (ours_bits != theirs_bits) half_mismatches += 1u;
  }
  CHECK(half_mismatches == 0u, "GcHalfToFloat differs from HalfToFloat for %u inputs", half_mismatches);
  CHECK(gc::GcHalfToFloat(0x0200u) == scene::HalfToFloat(0x0200u), "half subnormal 0x0200 matches the CPU");

  // A negative absolute index (raw 0, base -3) is counted as invalid.
  const uint8_t zero_index[4] = {0u, 0u, 0u, 0u};
  uint32_t invalid_range[4] = {};
  gc::GcRangeJob(zero_index, sizeof(zero_index), 0u, 2u, 2u, -3, invalid_range);
  CHECK(invalid_range[2] == 2u, "negative absolute indices counted: %u", invalid_range[2]);
}

// The carrier blocks of a job are contiguous, and an odd vertex count leaves the UV padding at 0.
static void TestGcCarrierLayout() {
  namespace gc = falcom_world::bvh;
  namespace rt12 = falcom_world::rt12;
  Stage("gpu capture carrier layout");
  static_assert(sizeof(gc::GcDecodeParams) == 64u);
  CHECK(gc::kGcCarrierWidth == rt12::kRt12CarrierWidth, "carrier width %u vs %u", gc::kGcCarrierWidth, rt12::kRt12CarrierWidth);
  struct Case {
    uint32_t vertices, indices;
    bool uv;
  };
  const Case cases[4] = {{1u, 3u, false}, {3u, 3u, true}, {4097u, 12u, true}, {5u, 4096u * 4u + 3u, false}};
  for (const Case& c : cases) {
    const uint32_t index_texels = (c.indices + 3u) / 4u;
    const uint32_t uv_begin = c.vertices + index_texels;
    const uint32_t uv_end = uv_begin + (c.uv ? (c.vertices + 1u) / 2u : 0u);
    CHECK(gc::GcJobTexels(c.vertices, c.indices, c.uv) == uv_end, "texels of (%u, %u, %d): %u, blocks end at %u", c.vertices, c.indices,
          c.uv ? 1 : 0, gc::GcJobTexels(c.vertices, c.indices, c.uv), uv_end);
    if (c.uv) CHECK(2u * ((c.vertices + 1u) / 2u) - c.vertices <= 1u, "UV pairs hold the vertices");
  }
  // Odd vertex count: the second UV of the last pair is never written and stays 0.
  uint8_t staging[256] = {};
  const float positions[9] = {0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 2.f, 2.f, 2.f};
  const float uv_values[6] = {0.25f, 0.5f, 0.75f, 1.f, 1.25f, 1.5f};
  std::memcpy(staging, positions, sizeof(positions));
  for (uint32_t v = 0u; v < 3u; ++v) std::memcpy(staging + 12u + 20u * v, uv_values + 2u * v, 8u);
  const uint16_t index_values[3] = {0u, 1u, 2u};
  std::memcpy(staging + 64u, index_values, sizeof(index_values));
  gc::GcDecodeParams params;
  params.index_byte = 64u;
  params.index_size = 2u;
  params.index_count = 3u;
  params.vertex_byte = 0u;
  params.stride = 20u;
  params.pos_format = gc::kGcFormatF32x3;
  params.uv_byte = 0u;
  params.uv_stride = 20u;
  params.uv_offset = 12u;
  params.uv_format = gc::kGcFormatF32x2;
  params.vertex_count = 3u;
  params.idx_texel = 3u;
  std::vector<uint32_t> carrier(6u * 4u, 0u);
  gc::GcDecodeJob(staging, sizeof(staging), params, carrier.data());
  const uint32_t padding_texel = 5u;  // uv block: texel 4 holds vertices 0 and 1, texel 5 holds vertex 2 and the padding
  CHECK(carrier[padding_texel * 4u + 2u] == 0u && carrier[padding_texel * 4u + 3u] == 0u, "UV padding of an odd vertex count is 0");
  float uv_pair[4] = {};
  std::memcpy(uv_pair, &carrier[4u * 4u], sizeof(uv_pair));  // texel 4: vertices 0 and 1
  CHECK(uv_pair[0] == 0.25f && uv_pair[1] == 0.5f && uv_pair[2] == 0.75f && uv_pair[3] == 1.f, "UV pair of vertices 0 and 1");
}

// GcBlasBudgetCount: the largest prefix within the budget, at least one.
static void TestGcBudget() {
  namespace gc = falcom_world::bvh;
  Stage("gpu capture budget");
  CHECK(gc::GcBlasBudgetCount({600000u, 600000u, 10u}, 1u << 20) == 1u, "first two over budget");
  CHECK(gc::GcBlasBudgetCount({10u, 20u, 30u}, 1u << 20) == 3u, "three small meshes");
  CHECK(gc::GcBlasBudgetCount({2000000u}, 1u << 20) == 1u, "one mesh over budget is built alone");
  CHECK(gc::GcBlasBudgetCount({}, 1u << 20) == 0u, "empty queue");
  CHECK(gc::GcBlasBudgetCount({1u << 20, 1u}, 1u << 20) == 1u, "exact fit then one more");
}

// GcCompareTriangles: bitwise per triangle, the first mismatch, counts, and UVs only when the CPU has them.
static void TestGcCompare() {
  namespace gc = falcom_world::bvh;
  Stage("gpu capture compare");
  const float gpu[12] = {0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f};  // float4 x 3
  const std::vector<std::array<float, 3>> cpu = {{0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}};
  const std::vector<uint32_t> cpu_indices = {0u, 1u, 2u};
  const uint32_t gpu_indices[3] = {0u, 1u, 2u};
  gc::GcCompareResult same = gc::GcCompareTriangles(gpu, 3u, gpu_indices, 3u, nullptr, cpu, cpu_indices, {});
  CHECK(same.mismatched == 0u && same.first < 0 && same.triangles == 1u, "identical triangle compares equal");

  float flipped[12] = {};
  std::memcpy(flipped, gpu, sizeof(gpu));
  uint32_t one_bit = 0u;
  std::memcpy(&one_bit, &flipped[0], sizeof(one_bit));
  one_bit ^= 1u;
  std::memcpy(&flipped[0], &one_bit, sizeof(one_bit));  // x of vertex 0 differs in one bit
  gc::GcCompareResult bit = gc::GcCompareTriangles(flipped, 3u, gpu_indices, 3u, nullptr, cpu, cpu_indices, {});
  CHECK(bit.mismatched == 1u && bit.first == 0, "one changed bit: mismatched %u first %lld", bit.mismatched, static_cast<long long>(bit.first));

  const uint32_t two_triangles[6] = {0u, 1u, 2u, 0u, 1u, 2u};
  gc::GcCompareResult counts = gc::GcCompareTriangles(gpu, 3u, two_triangles, 6u, nullptr, cpu, cpu_indices, {});
  CHECK(counts.mismatched == 2u && counts.first == 0, "different triangle counts: mismatched %u", counts.mismatched);

  const uint32_t out_of_range[3] = {0u, 1u, 9u};
  gc::GcCompareResult outside = gc::GcCompareTriangles(gpu, 3u, out_of_range, 3u, nullptr, cpu, cpu_indices, {});
  CHECK(outside.mismatched == 1u, "GPU index out of range is a mismatch");

  const float gpu_uvs[6] = {0.f, 0.f, 1.f, 0.f, 0.f, 1.f};
  const std::vector<std::array<float, 2>> cpu_uvs = {{0.f, 0.f}, {1.f, 0.f}, {0.f, 2.f}};
  gc::GcCompareResult no_cpu_uv = gc::GcCompareTriangles(gpu, 3u, gpu_indices, 3u, gpu_uvs, cpu, cpu_indices, {});
  CHECK(!no_cpu_uv.uv_compared && no_cpu_uv.mismatched == 0u, "no CPU UVs: not compared");
  gc::GcCompareResult with_uv = gc::GcCompareTriangles(gpu, 3u, gpu_indices, 3u, gpu_uvs, cpu, cpu_indices, cpu_uvs);
  CHECK(with_uv.uv_compared && with_uv.uv_mismatched == 1u && with_uv.mismatched == 1u, "UV mismatch counted once per triangle");
  const std::vector<std::array<float, 2>> short_uvs = {{0.f, 0.f}, {1.f, 0.f}};  // fewer UVs than positions
  gc::GcCompareResult short_uv = gc::GcCompareTriangles(gpu, 3u, gpu_indices, 3u, gpu_uvs, cpu, cpu_indices, short_uvs);
  CHECK(!short_uv.uv_compared && short_uv.uv_mismatched == 0u && short_uv.mismatched == 0u, "UVs shorter than positions: not compared");
}

// The draw side (bvh/gpu_capture.hpp): refusals, the creation counts, the phases, invalidation, and the OFF path.
static void TestGcHook(Device* dev) {
  namespace gc = falcom_world::bvh;
  Stage("gpu capture hook");
  constexpr uint64_t kGcLayout = 0x6A1u;
  constexpr uint64_t kGcLayoutUnorm = 0x6A2u;
  {
    renodx::utils::scene::InputLayoutInfo info;
    renodx::utils::scene::InputElementCopy position;
    position.semantic = "POSITION";
    position.format = reshade::api::format::r32g32b32_float;
    position.stride = 12u;
    info.elements.push_back(position);
    renodx::utils::scene::shared.data->input_layouts[kGcLayout] = info;
    renodx::utils::scene::InputLayoutInfo unorm;
    renodx::utils::scene::InputElementCopy color = position;
    color.format = reshade::api::format::r8g8b8a8_unorm;
    color.stride = 4u;
    unorm.elements.push_back(color);
    renodx::utils::scene::shared.data->input_layouts[kGcLayoutUnorm] = unorm;
  }
  const auto make_buffer = [dev](uint64_t bytes, reshade::api::resource* out) {
    reshade::api::resource_desc desc = {};
    desc.type = reshade::api::resource_type::buffer;
    desc.buffer.size = bytes;
    desc.heap = reshade::api::memory_heap::gpu_only;
    desc.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::copy_source | reshade::api::resource_usage::copy_dest;
    return dev->create_resource(desc, nullptr, reshade::api::resource_usage::shader_resource, out);
  };
  reshade::api::resource vb = {0u}, ib = {0u}, staging = {0u};
  CHECK(make_buffer(4096u, &vb) && make_buffer(4096u, &ib) && make_buffer(16u << 20, &staging), "test buffers");
  CmdList cl;
  cl.dev = dev;

  gc::ResetWorldPool();
  gc::g_gpu_capture.active.store(true);
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    gc::g_gpu_capture.staging = staging;
    gc::g_gpu_capture.staging_used = 0u;
  }
  const auto base_draw = [&](uint64_t vb_offset, uint32_t first_index, uint64_t layout) {
    falcom_world::DrawRecord draw;
    draw.method = 1;
    draw.has_index_buffer = true;
    draw.vb = vb;
    draw.vb_offset = vb_offset;
    draw.vb_stride = layout == kGcLayoutUnorm ? 4u : 12u;
    draw.vb_size = 4096u;
    draw.ib = ib;
    draw.ib_size = 4096u;
    draw.index_size = 2u;
    draw.index_count = 36u;
    draw.first_index = first_index;
    draw.instance_count = 1u;
    draw.input_layout = {layout};
    draw.topology = reshade::api::primitive_topology::triangle_list;
    return draw;
  };
  const falcom_world::DrawRecord draw_a = base_draw(0u, 0u, kGcLayout);
  const uint64_t key_a = gc::PoolMeshKey(draw_a);
  {
    std::lock_guard lock(gc::g_pool.mutex);
    gc::AddPoolResourceKey(draw_a.vb.handle, key_a);
    gc::AddPoolResourceKey(draw_a.ib.handle, key_a);
  }
  const gc::PoolUvStream no_uv;
  CHECK(gc::GpuCaptureOnDraw(&cl, draw_a, key_a, no_uv, false) == 1u, "first sighting copies the index range");
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    CHECK(gc::g_gpu_capture.entries.at(key_a).phase == gc::GcPhase::RangeWait && gc::g_gpu_capture.range_jobs.size() == 1u,
          "entry waits for the range result");
  }
  CHECK(gc::GpuCaptureOnDraw(&cl, draw_a, key_a, no_uv, false) == 0u, "second draw in the same frame copies nothing");
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    gc::GcEntry& entry = gc::g_gpu_capture.entries.at(key_a);
    entry.phase = gc::GcPhase::Data;
    entry.min_vertex = 0u;
    entry.max_vertex = 7u;
  }
  CHECK(gc::GpuCaptureOnDraw(&cl, draw_a, key_a, no_uv, false) == 2u, "data draw copies the vertex and index ranges");
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    CHECK(gc::g_gpu_capture.entries.at(key_a).phase == gc::GcPhase::DataWait && gc::g_gpu_capture.decode_jobs.size() == 1u,
          "entry waits for the decode");
  }

  // Deferred draws serve no copy.
  const falcom_world::DrawRecord draw_b = base_draw(0u, 36u, kGcLayout);
  const uint64_t key_b = gc::PoolMeshKey(draw_b);
  CHECK(gc::GpuCaptureOnDraw(&cl, draw_b, key_b, no_uv, true) == 0u, "deferred draw copies nothing");
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    CHECK(gc::g_gpu_capture.stats.deferred_skips == 1u, "deferred skip counted");
  }

  // Refusals: a written buffer, a dynamic buffer, a non-float position format.
  falcom_world::DrawRecord written = base_draw(64u, 0u, kGcLayout);
  written.vb_usage = static_cast<uint32_t>(reshade::api::resource_usage::unordered_access);
  const uint64_t key_written = gc::PoolMeshKey(written);
  CHECK(gc::GpuCaptureOnDraw(&cl, written, key_written, no_uv, false) == 0u, "written buffer copies nothing");
  falcom_world::DrawRecord dynamic = base_draw(128u, 0u, kGcLayout);
  dynamic.vb_flags = static_cast<uint32_t>(reshade::api::resource_flags::dynamic);
  const uint64_t key_dynamic = gc::PoolMeshKey(dynamic);
  CHECK(gc::GpuCaptureOnDraw(&cl, dynamic, key_dynamic, no_uv, false) == 0u, "dynamic buffer copies nothing");
  falcom_world::DrawRecord unorm = base_draw(256u, 0u, kGcLayoutUnorm);
  const uint64_t key_unorm = gc::PoolMeshKey(unorm);
  CHECK(gc::GpuCaptureOnDraw(&cl, unorm, key_unorm, no_uv, false) == 0u, "unorm position copies nothing");
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    const auto& refused = gc::g_gpu_capture.stats.refused;
    CHECK(refused.count("gpu_written") == 1u && refused.at("gpu_written") == 1u, "gpu_written counted");
    CHECK(refused.count("cpu_dynamic") == 1u && refused.at("cpu_dynamic") == 1u, "cpu_dynamic counted");
    CHECK(refused.count("format") == 1u && refused.at("format") == 1u, "format counted");
    CHECK(gc::g_gpu_capture.entries.at(key_unorm).phase == gc::GcPhase::Refused, "unorm entry refused");
  }

  // Writes: three writes refuse the key; the key is reported for the D3D12 side.
  gc::InvalidateGpuCaptureKeys({key_a}, false);
  gc::InvalidateGpuCaptureKeys({key_a}, false);
  gc::InvalidateGpuCaptureKeys({key_a}, false);
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    CHECK(gc::g_gpu_capture.entries.at(key_a).phase == gc::GcPhase::Refused, "third write refuses the key");
    CHECK(gc::g_gpu_capture.stats.refused.count("rewritten") == 1u, "rewritten counted");
    CHECK(std::find(gc::g_gpu_capture.invalidated.begin(), gc::g_gpu_capture.invalidated.end(), key_a) != gc::g_gpu_capture.invalidated.end(),
          "invalidated key reported");
  }
  // Destroy: the entry is erased.
  gc::InvalidateGpuCaptureKeys({key_a}, true);
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    CHECK(gc::g_gpu_capture.entries.count(key_a) == 0u, "destroyed buffer erases its entry");
  }
  gc::ResetGpuCaptureEntries("pool reset");
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    CHECK(gc::g_gpu_capture.entries.empty() && gc::g_gpu_capture.stats.resets_pool == 1u, "pool reset empties the cache");
  }

  // OFF path: with the cache inactive the pool hook creates no entries.
  gc::g_gpu_capture.active.store(false);
  uint64_t created_before = 0u;
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    created_before = gc::g_gpu_capture.stats.created;
  }
  const bool scan_was = gc::g_pool.scan_active.load();
  gc::g_pool.scan_active.store(true);
  falcom_world::WorldCommandListData cl_data;
  gc::OnPoolScanDraw(dev, &cl, draw_a, &cl_data);
  gc::g_pool.scan_active.store(scan_was);
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    CHECK(gc::g_gpu_capture.entries.empty() && gc::g_gpu_capture.stats.created == created_before, "OFF path creates no cache objects");
  }
  // Restore.
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    gc::g_gpu_capture.staging = {0u};
    gc::g_gpu_capture.staging_used = 0u;
    gc::g_gpu_capture.invalidated.clear();
    gc::g_gpu_capture.stats = gc::GcStats{};
  }
  gc::ResetWorldPool();
  dev->destroy_resource(vb);
  dev->destroy_resource(ib);
  dev->destroy_resource(staging);
}

// G3 (gpu_capture.hpp GcPoseHash): the pose key is the grid of the 12 world floats (translation 1/256 m, basis 1/4096).
static void TestGcPoseHash() {
  Stage("gpu instance pose hash");
  const auto key = [](uint32_t lead, const float* world) {
    uint32_t out[2] = {};
    falcom_world::bvh::GcPoseHash(lead, world, out);
    return (uint64_t{out[0]} << 32u) | out[1];
  };
  const float base[12] = {1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 2.f, 0.f, 0.f, 1.f, 3.f};
  float near_basis[12] = {};
  std::memcpy(near_basis, base, sizeof(base));
  near_basis[0] += 3e-8f;
  float moved[12] = {};
  std::memcpy(moved, base, sizeof(base));
  moved[7] += 0.005f;
  const float identity[12] = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f};
  const float negative_zero[12] = {1.f, 0.f, 0.f, -0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f};
  const float half_step[12] = {1.f, 0.f, 0.f, 0.5f / 256.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f};
  const float tie_up[12] = {1.f, 0.f, 0.f, 1.5f / 256.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f};
  const float two_step[12] = {1.f, 0.f, 0.f, 2.f / 256.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f};
  CHECK(key(1, base) == key(1, base), "same world, same key");
  CHECK(key(1, base) == key(1, near_basis), "basis +3e-8 keeps the key");
  CHECK(key(1, base) != key(1, moved), "translation +0.005 m changes the key");
  CHECK(key(1, base) != key(2, base), "the leading word (mesh slot or mesh id) is part of the key");
  CHECK(key(1, identity) == key(1, negative_zero), "-0 and 0 share a key");
  CHECK(key(1, identity) == key(1, half_step), "translation 0.5/256 ties to even (0)");
  CHECK(key(1, tie_up) == key(1, two_step), "translation 1.5/256 ties to even (2)");
  std::set<uint32_t> low16;
  std::set<uint32_t> low6;
  for (int i = 0; i < 100; ++i) {
    const float along[12] = {1.f, 0.f, 0.f, static_cast<float>(i), 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f};
    uint32_t out[2] = {};
    falcom_world::bvh::GcPoseHash(6u, along, out);
    low16.insert(out[0] & 0xFFFFu);
    low6.insert(out[0] & 63u);
  }
  CHECK(low16.size() >= 95u, "finalizer: 100 poses give >= 95 distinct low16 (%zu)", low16.size());
  CHECK(low6.size() >= 40u, "finalizer: 100 poses give >= 40 distinct low6 (%zu)", low6.size());
}

// G3 draw side (D2) and the bookkeeping of slots and epochs (D1) on the mock device.
static void TestGcInstanceHook(Device* dev) {
  namespace gc = falcom_world::bvh;
  Stage("gpu instance hook");
  constexpr uint64_t kGcLayout = 0x6A1u;
  {
    renodx::utils::scene::InputLayoutInfo info;
    renodx::utils::scene::InputElementCopy position;
    position.semantic = "POSITION";
    position.format = reshade::api::format::r32g32b32_float;
    position.stride = 12u;
    info.elements.push_back(position);
    renodx::utils::scene::shared.data->input_layouts[kGcLayout] = info;
  }
  const auto make_buffer = [dev](uint64_t bytes, reshade::api::resource* out) {
    reshade::api::resource_desc desc = {};
    desc.type = reshade::api::resource_type::buffer;
    desc.buffer.size = bytes;
    desc.heap = reshade::api::memory_heap::gpu_only;
    desc.usage = reshade::api::resource_usage::shader_resource | reshade::api::resource_usage::copy_source | reshade::api::resource_usage::copy_dest;
    return dev->create_resource(desc, nullptr, reshade::api::resource_usage::shader_resource, out);
  };
  reshade::api::resource vb = {0u}, ib = {0u}, staging = {0u}, slices = {0u}, arena = {0u};
  CHECK(make_buffer(4096u, &vb) && make_buffer(4096u, &ib) && make_buffer(16u << 20, &staging) && make_buffer(160u * 8u, &slices)
            && make_buffer(1u << 20, &arena),
        "test buffers");
  CmdList cl;
  cl.dev = dev;
  const auto base_draw = [&](uint64_t vb_offset, uint32_t first_index) {
    falcom_world::DrawRecord draw;
    draw.method = 1;
    draw.has_index_buffer = true;
    draw.vb = vb;
    draw.vb_offset = vb_offset;
    draw.vb_stride = 12u;
    draw.vb_size = 4096u;
    draw.ib = ib;
    draw.ib_size = 4096u;
    draw.index_size = 2u;
    draw.index_count = 36u;
    draw.first_index = first_index;
    draw.instance_count = 1u;
    draw.input_layout = {kGcLayout};
    draw.topology = reshade::api::primitive_topology::triangle_list;
    return draw;
  };

  gc::ResetWorldPool();
  gc::g_gpu_capture.active.store(true);
  uint64_t slot_version_reset = 0u;
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    gc::g_gpu_capture.settle_reset = false;
    slot_version_reset = gc::g_gpu_capture.slot_version;
  }
  gc::ResetGpuCaptureEntries("pool reset");
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    CHECK(gc::g_gpu_capture.settle_reset && gc::g_gpu_capture.slot_version == slot_version_reset + 1u,
          "a pool reset sets settle_reset and raises slot_version by 1");
    gc::g_gpu_capture.settle_reset = false;
  }
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    gc::g_gpu_capture.staging = staging;
    gc::g_gpu_capture.staging_used = 0u;
    gc::g_gpu_capture.instance_arena = arena;
    gc::g_gpu_capture.instance_arena_used = 0u;
    gc::g_gpu_capture.instance_draws.clear();
  }
  const falcom_world::DrawRecord draw = base_draw(0u, 0u);
  const uint64_t key = gc::PoolMeshKey(draw);
  {
    std::lock_guard lock(gc::g_pool.mutex);
    gc::AddPoolResourceKey(draw.vb.handle, key);
    gc::AddPoolResourceKey(draw.ib.handle, key);
  }
  const gc::PoolUvStream no_uv;
  const gc::GcInstanceSource source = {.buffer = slices, .base = 2, .count = 3u, .pass = 9u};

  // a. The first sighting creates the entry (Range): no instance copy.
  uint64_t slot_version_before_create = 0u;
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    slot_version_before_create = gc::g_gpu_capture.slot_version;
  }
  gc::GpuCaptureOnDraw(&cl, draw, key, no_uv, false, &source);
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    const gc::GcEntry& entry = gc::g_gpu_capture.entries.at(key);
    CHECK(gc::g_gpu_capture.slot_version == slot_version_before_create + 1u, "creating an entry raises slot_version by 1");
    CHECK(entry.phase == gc::GcPhase::RangeWait, "first sighting waits for its range result");
    CHECK(gc::g_gpu_capture.stats.inst_no_geometry == 1u, "no geometry counted (%llu)", gc::g_gpu_capture.stats.inst_no_geometry);
    CHECK(gc::g_gpu_capture.instance_draws.empty(), "no instance draw before Done");
    CHECK(gc::g_gpu_capture.slot_key[entry.slot] == key, "the entry's slot maps to its key");
  }

  // b. Done: the slice is copied and recorded with its slot and epoch.
  uint32_t slot = 0u;
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    gc::GcEntry& entry = gc::g_gpu_capture.entries.at(key);
    entry.phase = gc::GcPhase::Done;
    slot = entry.slot;
  }
  CHECK(gc::GpuCaptureOnDraw(&cl, draw, key, no_uv, false, &source) == 1u, "Done draw copies its instance slice");
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    CHECK(gc::g_gpu_capture.instance_draws.size() == 1u, "one instance draw recorded");
    const gc::GcInstanceDrawGPU& recorded = gc::g_gpu_capture.instance_draws[0];
    CHECK(recorded.arena_float4 == 0u && recorded.count == 3u && recorded.slot == slot, "draw: arena 0, count 3, slot %u", recorded.slot);
    CHECK(recorded.epoch == gc::g_gpu_capture.slot_epoch[slot], "draw epoch is the slot epoch");
    CHECK(recorded.pass_bits == 9u, "draw pass bits");
    CHECK(recorded.args_float4 == gc::kGcNoArgs, "a direct draw keeps the direct path (args_float4 %u)", recorded.args_float4);
    CHECK(gc::g_gpu_capture.instance_arena_used == 480u, "arena used 480 (%llu)", gc::g_gpu_capture.instance_arena_used);
    CHECK(gc::g_gpu_capture.stats.inst_draws == 1u && gc::g_gpu_capture.stats.inst_instances == 3u, "draw counters");
  }

  // c. Deferred: no copy.
  CHECK(gc::GpuCaptureOnDraw(&cl, draw, key, no_uv, true, &source) == 0u, "deferred instance draw copies nothing");
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    CHECK(gc::g_gpu_capture.stats.inst_deferred == 1u, "deferred counted");
  }

  // d. Arena full.
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    gc::g_gpu_capture.instance_arena_used = gc::kGcInstanceArenaBytes - 100u;
  }
  CHECK(gc::GpuCaptureOnDraw(&cl, draw, key, no_uv, false, &source) == 0u, "full arena copies nothing");
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    CHECK(gc::g_gpu_capture.stats.inst_arena_full == 1u, "arena full counted");
    gc::g_gpu_capture.instance_arena_used = 480u;
  }

  // e. Draw cap.
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    gc::g_gpu_capture.instance_draws.resize(gc::kGcInstanceDrawsPerPresent);
  }
  CHECK(gc::GpuCaptureOnDraw(&cl, draw, key, no_uv, false, &source) == 0u, "draw cap copies nothing");
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    CHECK(gc::g_gpu_capture.stats.inst_draws_cap == 1u, "draw cap counted");
    gc::g_gpu_capture.instance_draws.clear();
  }

  // e2. G3b: an indirect sub-draw copies its 20 args bytes into a 32-byte slot before the window.
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    gc::g_gpu_capture.instance_draws.clear();
    gc::g_gpu_capture.instance_arena_used = 0u;
  }
  const gc::GcInstanceSource indirect = {.buffer = slices, .base = 2, .count = 4u, .pass = 8u, .args = ib, .args_offset = 40u};
  CHECK(gc::GpuCaptureOnDraw(&cl, draw, key, no_uv, false, &indirect) == 2u, "indirect draw copies its args and its window");
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    CHECK(gc::g_gpu_capture.instance_draws.size() == 1u, "one indirect instance draw recorded");
    const gc::GcInstanceDrawGPU& recorded = gc::g_gpu_capture.instance_draws[0];
    CHECK(recorded.args_float4 == 0u && recorded.arena_float4 == 2u && recorded.count == 4u, "indirect draw: args 0, arena 2, count 4 (%u, %u, %u)",
          recorded.args_float4, recorded.arena_float4, recorded.count);
    CHECK(recorded.index_count == 36u && recorded.first_index == 0u && recorded.vertex_offset == 0,
          "indirect draw carries the learned range (%u, %u, %d)", recorded.index_count, recorded.first_index, recorded.vertex_offset);
    CHECK(gc::g_gpu_capture.instance_arena_used == 672u, "indirect arena used 672 (%llu)", gc::g_gpu_capture.instance_arena_used);
    CHECK(gc::g_gpu_capture.stats.inst_indirect_draws == 1u, "inst_indirect_draws 1 (%llu)", gc::g_gpu_capture.stats.inst_indirect_draws);
  }
  // The args and the window must fit together: 640 fits, 672 does not.
  uint64_t full_before = 0u;
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    gc::g_gpu_capture.instance_draws.clear();
    gc::g_gpu_capture.instance_arena_used = gc::kGcInstanceArenaBytes - 650u;
    full_before = gc::g_gpu_capture.stats.inst_arena_full;
  }
  CHECK(gc::GpuCaptureOnDraw(&cl, draw, key, no_uv, false, &indirect) == 0u, "indirect draw that does not fit copies nothing");
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    CHECK(gc::g_gpu_capture.stats.inst_arena_full == full_before + 1u, "indirect arena full counted");
    gc::g_gpu_capture.instance_draws.clear();
    gc::g_gpu_capture.instance_arena_used = 480u;
  }

  // f. A write bumps the slot epoch, keeps the slot.
  uint32_t epoch_before = 0u;
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    epoch_before = gc::g_gpu_capture.slot_epoch[slot];
    gc::g_gpu_capture.epochs_dirty = false;
  }
  gc::InvalidateGpuCaptureKeys({key}, false);
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    CHECK(gc::g_gpu_capture.slot_epoch[slot] == epoch_before + 1u, "write bumps the epoch");
    CHECK(gc::g_gpu_capture.epochs_dirty, "epoch change marks the table dirty");
    const gc::GcEntry& entry = gc::g_gpu_capture.entries.at(key);
    CHECK(entry.slot == slot && entry.phase == gc::GcPhase::Range, "write keeps the slot and restarts the entry");
  }

  // g. Destroy frees the slot and bumps the epoch again.
  gc::InvalidateGpuCaptureKeys({key}, true);
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    CHECK(gc::g_gpu_capture.entries.count(key) == 0u, "destroy erases the entry");
    CHECK(std::find(gc::g_gpu_capture.free_slots.begin(), gc::g_gpu_capture.free_slots.end(), slot) != gc::g_gpu_capture.free_slots.end(),
          "destroy frees the slot");
    CHECK(gc::g_gpu_capture.slot_key[slot] == 0u, "freed slot has no key");
    CHECK(gc::g_gpu_capture.slot_epoch[slot] == epoch_before + 2u, "destroy bumps the epoch (%u)", gc::g_gpu_capture.slot_epoch[slot]);
  }

  // h. A new key takes the freed slot.
  CHECK(gc::GpuCaptureOnDraw(&cl, draw, key, no_uv, false, &source) == 1u, "new entry copies its range");
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    CHECK(gc::g_gpu_capture.entries.at(key).slot == slot, "new entry reuses the freed slot");
    CHECK(gc::g_gpu_capture.slot_key[slot] == key, "reused slot maps to the new key");
    CHECK(gc::g_gpu_capture.free_slots.empty(), "free list consumed");
  }

  // i. A key refused at creation (written buffer) bumps the epoch of its slot.
  falcom_world::DrawRecord written = base_draw(64u, 0u);
  written.vb_usage = static_cast<uint32_t>(reshade::api::resource_usage::unordered_access);
  const uint64_t key_written = gc::PoolMeshKey(written);
  uint32_t expected_slot = 0u;
  uint32_t written_before = 0u;
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    expected_slot = gc::g_gpu_capture.free_slots.empty() ? gc::g_gpu_capture.next_slot : gc::g_gpu_capture.free_slots.back();
    written_before = gc::g_gpu_capture.slot_epoch[expected_slot];
  }
  gc::GpuCaptureOnDraw(&cl, written, key_written, no_uv, false);
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    const gc::GcEntry& entry = gc::g_gpu_capture.entries.at(key_written);
    CHECK(entry.phase == gc::GcPhase::Refused, "written buffer refused at creation");
    CHECK(entry.slot == expected_slot && gc::g_gpu_capture.slot_epoch[entry.slot] == written_before + 1u,
          "refusal bumps the slot epoch (slot %u)", entry.slot);
  }

  // j. A pool reset returns every slot.
  gc::ResetGpuCaptureEntries("pool reset");
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    CHECK(gc::g_gpu_capture.next_slot == 0u && gc::g_gpu_capture.free_slots.empty(), "reset returns the slots");
    CHECK(gc::g_gpu_capture.entries.empty(), "reset empties the entries");
  }

  // j2. G4-0 (0a): a direct draw with no entry is noted; its creation erases the note; a reset clears the notes.
  const falcom_world::DrawRecord noted = base_draw(128u, 0u);
  const uint64_t key_noted = gc::PoolMeshKey(noted);
  gc::GpuCaptureNoteNoEntry(key_noted, "untracked");
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    CHECK(gc::g_gpu_capture.no_entry_reason.count(key_noted) == 1u
              && std::strcmp(gc::g_gpu_capture.no_entry_reason.at(key_noted), "untracked") == 0,
          "no-entry note recorded");
  }
  gc::GpuCaptureOnDraw(&cl, noted, key_noted, no_uv, false);
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    CHECK(gc::g_gpu_capture.no_entry_reason.count(key_noted) == 0u, "creation erases the no-entry note");
  }
  gc::GpuCaptureNoteNoEntry(key_noted, "cap");
  gc::ResetGpuCaptureEntries("pool reset");
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    CHECK(gc::g_gpu_capture.no_entry_reason.empty(), "reset clears the no-entry notes");
  }

  // j3. G4-0 (T-1): a deferred Range draw with instances counts inst_deferred.
  const falcom_world::DrawRecord deferred_range = base_draw(192u, 0u);
  CHECK(gc::GpuCaptureOnDraw(&cl, deferred_range, gc::PoolMeshKey(deferred_range), no_uv, true, &source) == 0u,
        "deferred range instance draw copies nothing");
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    CHECK(gc::g_gpu_capture.stats.inst_deferred == 2u, "deferred range instance draw counted (%llu)", gc::g_gpu_capture.stats.inst_deferred);
  }

  // k. OFF path: an inactive cache copies nothing and records nothing.
  gc::g_gpu_capture.active.store(false);
  CHECK(gc::GpuCaptureOnDraw(&cl, draw, key, no_uv, false, &source) == 0u, "inactive cache copies nothing");
  gc::GpuCaptureNoteNoEntry(key_noted, "untracked");
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    CHECK(gc::g_gpu_capture.no_entry_reason.empty(), "inactive cache notes no entry");
  }
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    CHECK(gc::g_gpu_capture.instance_draws.empty(), "inactive cache records no instance draw");
  }

  // Restore.
  {
    std::lock_guard lock(gc::g_gpu_capture.mutex);
    gc::g_gpu_capture.instance_draws.clear();
    gc::g_gpu_capture.instance_arena = {0u};
    gc::g_gpu_capture.instance_arena_used = 0u;
    gc::g_gpu_capture.staging = {0u};
    gc::g_gpu_capture.staging_used = 0u;
    gc::g_gpu_capture.invalidated.clear();
    gc::g_gpu_capture.range_jobs.clear();
    gc::g_gpu_capture.decode_jobs.clear();
    gc::g_gpu_capture.stats = gc::GcStats{};
  }
  gc::ResetWorldPool();
  dev->destroy_resource(vb);
  dev->destroy_resource(ib);
  dev->destroy_resource(staging);
  dev->destroy_resource(slices);
  dev->destroy_resource(arena);
}

// G3 compare (D10): buckets of matched, GPU-only and CPU-only instances, with the reasons.
static void TestGcCompareInstances() {
  namespace gc = falcom_world::bvh;
  Stage("gpu instance compare");
  const auto place = [](float* world, float x, float y, float z) {
    const float m[12] = {1.f, 0.f, 0.f, x, 0.f, 1.f, 0.f, y, 0.f, 0.f, 1.f, z};
    std::memcpy(world, m, sizeof(m));
  };
  const auto cpu_at = [&place](uint64_t mesh_key, uint32_t mesh_id, float x, float y, float z, bool dynamic, bool recently_seen) {
    gc::GcCompareCpuInstance instance;
    instance.mesh_key = mesh_key;
    instance.mesh_id = mesh_id;
    place(instance.world, x, y, z);
    instance.dynamic = dynamic;
    instance.recently_seen = recently_seen;
    return instance;
  };
  std::vector<gc::GcCompareGpuInstance> gpu(6);
  gpu[0].mesh_key = 0x1u;
  gpu[0].mesh_id = 1;
  place(gpu[0].world, 1.f, 2.f, 3.f);
  gpu[0].flags = 2u;
  gpu[1].mesh_key = 0x9u;  // the same content as gpu[0] under another key
  gpu[1].mesh_id = 1;
  place(gpu[1].world, 1.f, 2.f, 3.f);
  gpu[1].flags = 2u;
  gpu[2].mesh_key = 0x2u;
  gpu[2].mesh_id = 2;
  place(gpu[2].world, 5.f, 0.f, 0.f);
  gpu[2].flags = 16u;
  gpu[2].cpu_alpha_conflict = true;
  gpu[2].cpu_state = "not_observed";
  gpu[3].mesh_key = 0x4u;
  gpu[3].mesh_id = -1;
  place(gpu[3].world, 0.f, 0.f, 0.f);
  gpu[3].cpu_state = "absent";
  gpu[4].mesh_key = 0x10u;  // no CPU mesh: the pool failed it
  gpu[4].mesh_id = -1;
  place(gpu[4].world, 2.f, 0.f, 0.f);
  gpu[4].cpu_state = "failed:x";
  gpu[5].mesh_key = 0x11u;  // GPU-only, the CPU pool has it observed
  gpu[5].mesh_id = 9;
  place(gpu[5].world, 9.f, 9.f, 9.f);
  gpu[5].cpu_state = "observing";
  gpu[4].flags = 2u;
  gpu[5].flags = 2u;
  std::vector<gc::GcCompareCpuInstance> cpu = {
      cpu_at(0x1u, 1u, 1.f, 2.f, 3.f, false, false),   // matched
      cpu_at(0x1u, 1u, 1.f, 2.f, 3.f, false, false),   // the same instance again
      cpu_at(0x3u, 3u, 7.f, 0.f, 0.f, false, true),    // done
      cpu_at(0x8u, 4u, 9.f, 0.f, 0.f, true, false),    // dynamic
      cpu_at(0x5u, 5u, 11.f, 0.f, 0.f, false, false),  // no entry
      cpu_at(0x6u, 6u, 13.f, 0.f, 0.f, false, false),  // refused
      cpu_at(0x7u, 7u, 15.f, 0.f, 0.f, false, false),  // done, not seen recently
      cpu_at(0x12u, 8u, 17.f, 0.f, 0.f, false, false),  // no entry, indirect and untracked (G3b)
  };
  const std::unordered_map<uint64_t, std::string> reasons = {{0x3u, "done"}, {0x5u, "no_entry_indirect"}, {0x6u, "refused:format"}, {0x7u, "done"},
                                                             {0x12u, "no_entry_indirect_untracked"}};
  const gc::GcInstanceCompare result = gc::GcCompareInstances(gpu, cpu, reasons);
  CHECK(!result.camera_valid && result.gpu_only_camera_by_distance.empty() && result.gpu_no_cpu_mesh_camera_by_distance.empty()
            && result.cpu_only_by_distance.empty(),
        "without a camera no distance map is counted");
  CHECK(result.matched == 1u, "matched %llu", result.matched);
  CHECK(result.gpu_dup_content == 1u, "gpu duplicate content %llu", result.gpu_dup_content);
  CHECK(result.gpu_no_cpu_mesh == 2u, "gpu without a cpu mesh %llu", result.gpu_no_cpu_mesh);
  CHECK(result.gpu_only == 2u && result.gpu_only_alpha == 1u && result.gpu_only_alpha_conflict == 1u,
        "gpu-only %llu, alpha %llu, conflict %llu", result.gpu_only, result.gpu_only_alpha, result.gpu_only_alpha_conflict);
  CHECK(result.cpu_dup == 1u, "cpu duplicate %llu", result.cpu_dup);
  CHECK(result.cpu_only == 6u, "cpu-only %llu", result.cpu_only);
  const auto by_reason = [&result](const char* reason) {
    const auto it = result.cpu_only_by_reason.find(reason);
    return it == result.cpu_only_by_reason.end() ? uint64_t{0u} : it->second;
  };
  CHECK(by_reason("done") == 1u && by_reason("dynamic") == 1u && by_reason("no_entry_indirect") == 1u && by_reason("no_entry") == 0u
            && by_reason("no_entry_indirect_untracked") == 1u && by_reason("refused:format") == 1u && by_reason("done_unseen") == 1u
            && result.cpu_only_by_reason.size() == 6u,
        "reasons: done %llu, dynamic %llu, no_entry_indirect %llu, no_entry %llu, refused %llu, done_unseen %llu", by_reason("done"),
        by_reason("dynamic"), by_reason("no_entry_indirect"), by_reason("no_entry"), by_reason("refused:format"), by_reason("done_unseen"));
  CHECK(result.cpu_only_samples.size() == 6u && result.gpu_only_samples.size() == 2u, "samples %zu, %zu", result.cpu_only_samples.size(),
        result.gpu_only_samples.size());
  CHECK(result.gpu_no_cpu_mesh_by_state.size() == 2u && result.gpu_no_cpu_mesh_by_state.at("absent") == 1u
            && result.gpu_no_cpu_mesh_by_state.at("failed:x") == 1u,
        "no-cpu-mesh states: absent and failed:x");
  CHECK(result.gpu_only_by_state.size() == 2u && result.gpu_only_by_state.at("not_observed") == 1u
            && result.gpu_only_by_state.at("observing") == 1u,
        "gpu-only states: not_observed and observing");
  const float camera[3] = {0.f, 0.f, 0.f};
  const gc::GcInstanceCompare near_result = gc::GcCompareInstances(gpu, cpu, reasons, camera);
  CHECK(near_result.camera_valid, "a camera is recorded");
  CHECK(near_result.gpu_only_camera_by_distance.size() == 1u && near_result.gpu_only_camera_by_distance.at("lt16m") == 1u,
        "camera-visible GPU-only records by distance: lt16m 1");
  CHECK(near_result.gpu_no_cpu_mesh_camera_by_distance.size() == 1u && near_result.gpu_no_cpu_mesh_camera_by_distance.at("lt16m") == 1u,
        "camera-visible no-CPU-mesh records by distance: lt16m 1");
  CHECK(near_result.cpu_only_by_distance.size() == 2u && near_result.cpu_only_by_distance.at("lt16m") == 5u && near_result.cpu_only_by_distance.at("lt64m") == 1u,
        "CPU-only instances by distance: lt16m 5, lt64m 1");
}

static void TestRt12GcMeshWords() {
  namespace rt12 = falcom_world::rt12;
  Stage("rt12 gc mesh words");
  float words[16] = {};
  CHECK(rt12::Rt12GcMeshWords(5u, 10u, 100u, 30u, true, 7u, words), "exact offsets are accepted");
  CHECK(words[0] == 5.f && words[1] == 10.f && words[2] == 100.f && words[3] == 30.f && words[7] == 8.f,
        "header words and the UV offset + 1 (word 7)");
  bool others_zero = true;
  for (uint32_t k = 0u; k < 16u; ++k) {
    if (k != 0u && k != 1u && k != 2u && k != 3u && k != 7u && words[k] != 0.f) others_zero = false;
  }
  CHECK(others_zero, "every other word is 0");
  CHECK(rt12::Rt12GcMeshWords(5u, 10u, 100u, 30u, false, 7u, words) && words[7] == 0.f, "without UV word 7 is 0");
  CHECK(!rt12::Rt12GcMeshWords(5u, 10u, 1u << 24u, 30u, false, 0u, words), "an index offset of 2^24 is not exact");
  CHECK(!rt12::Rt12GcMeshWords(5u, 10u, 100u, 30u, true, (1u << 24u) - 5u, words), "a UV offset near 2^24 is not exact");
}

static void TestGcRecordsBound() {
  namespace gc = falcom_world::bvh;
  Stage("gpu records bound");
  CHECK(gc::GcRecordsBound({}) == 0u, "no counter read: bound 0");
  CHECK(gc::GcRecordsBound({0u}) == 2048u, "out 0: bound 2048");
  CHECK(gc::GcRecordsBound({10000u}) == 13312u, "out 10000: bound 13312");
  CHECK(gc::GcRecordsBound({80000u}) == 73728u, "out 80000: capped at the records buffer");
  CHECK(gc::GcRecordsBound({1000u, 9000u}) == 11264u, "the largest of the history counts");
}

int main() {
  falcom_world::g_state.pool_capture_requested.store(true);
  std::setvbuf(stdout, nullptr, _IONBF, 0);
  Stage("main start");
  CHECK(!bvh::g_perf_gate.gather.load(), "performance gathering is OFF by default");
  bvh::g_perf_gate.gather.store(true);  // the suites run with gathering on; TestPerfGateOff switches it off
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
  TestSortExact();
  TestRefitGolden();
  TestRefitCursorInterleaved();
  TestRefitCursorMiss();
  TestWindRangeGolden();
  TestWindRangeGate();
  TestRestWindGolden();
  TestRebuildGolden();
  TestVisibilityCarry();
  TestRestWindFastSlowEqual();
  TestFollowedWindCrossing();
  TestRestWindLag();
  TestRemoveFarWind();
  TestRefitGpuLeaves();
  TestRestPromotion();
  TestRestSteadyReads();
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
  TestPerfSpike(&dev);
  TestPerfSeriesCap(&dev);
  TestPerfAbort(&dev);
  TestPerfSwitchesStable(&dev);
  TestPerfTab(&dev);
  TestPerfExtraGpu(&dev);
  TestPerfUnattributed();
  TestPerfGateOff(&dev, &queue);
  TestPerfJsonKeys(&dev);
  TestPerfTimerGate();
  TestPerfCaptureInDrain(&dev);
  TestPerfSampleAlignment(&dev);
  TestPerfFrameExchange(&dev, &queue);
  TestPerfBlockGate();
  TestPerfBlockSample(&dev);
  TestPerfDisjointBlocks();
  TestPerfBlockAttribution(&dev);
  TestPerfWorstPresent(&dev);
  TestPerfSpikeTab();
  TestPerfSettingsCost(&dev);
  TestPerfFoliageMotion();
  TestPerfInclusionFlip(&dev);
  Stage("11e BVH range coverage");
  TestRegionCoverage(&dev, &queue);
  TestRt12Mirror(&dev, &queue);
  TestRt12Dynamic();
  TestRt12Records();
  TestRt12GcMeshWords();
  Stage("gpu capture");
  TestGcDecode();
  TestGcCarrierLayout();
  TestGcBudget();
  TestGcCompare();
  TestGcHook(&dev);
  TestGcPoseHash();
  TestGcInstanceHook(&dev);
  TestGcCompareInstances();
  TestGcRecordsBound();
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
