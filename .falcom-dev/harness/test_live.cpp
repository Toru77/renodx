// Native harness: live BVH. Mesh store streaming under the triangle budget,
// arena growth, TLAS rebuild timing and reasons, region moves, retirement,
// garbage reset, GPU allocation failure and retry, switches, failed meshes,
// the lock rule, the GPU contents check, and a CPU port of the trace shader
// run over the uploaded buffers and compared with brute force.
#include <cmath>
#include <cstdio>
#include <map>
#include <random>
#include <set>
#include "gen/mock_base.hpp"
#include "src/games/falcomengine-plus/world/bvh/bvh_live.hpp"

using namespace reshade::api;
namespace bvh = falcom_world::bvh;

static int g_failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++g_failures; std::printf("FAIL %s:%d %s | ", __FILE__, __LINE__, #cond); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

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
};
struct Counters { uint32_t invalid = 0, overflow = 0; };

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
static float TraceWorld(const TraceInputs& in, F3 o, F3 d, Counters& c, uint32_t* hit_instance, Fade fade = {},
                        uint32_t* camera_faded = nullptr) {
  *hit_instance = 0xFFFFFFFFu;
  uint32_t best_faded = 0u;
  float faded_t = 3.0e38f;
  if (camera_faded) *camera_faded = 0u;
  const uint32_t tl = in.tlas_leaves.count;
  if (tl == 0) return -1.f;
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
                else {
                  const float* iw = inst.inverse_world;
                  const F3 lo = {o.x * iw[0] + o.y * iw[1] + o.z * iw[2] + iw[3], o.x * iw[4] + o.y * iw[5] + o.z * iw[6] + iw[7], o.x * iw[8] + o.y * iw[9] + o.z * iw[10] + iw[11]};
                  const F3 ld = {d.x * iw[0] + d.y * iw[1] + d.z * iw[2], d.x * iw[4] + d.y * iw[5] + d.z * iw[6], d.x * iw[8] + d.y * iw[9] + d.z * iw[10]};
                  float shown_min = 0.f, shown_max = 3.0e38f;
                  const uint32_t visibility_flags = static_cast<uint32_t>(inst.visibility[2] + 0.5f);
                  if ((visibility_flags & 2u) == 0u) { shown_min = 3.0e38f; shown_max = -1.0f; }
                  else if ((visibility_flags & 1u) != 0u) HlslCameraFadeInterval(inst.visibility[0], inst.visibility[1], fade.floor_value, &shown_min, &shown_max);
                  float local_best = best;
                  uint32_t local_faded = 0u;
                  if (TraceBlas(in, mesh_id, lo, ld, 0.001f, &local_best, c, shown_min, shown_max, fade.hide, &faded_t, &local_faded)) {
                    best = local_best; any = true; *hit_instance = resolved; best_faded = local_faded;
                  }
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

// Brute force over the pool's instances in the region whose mesh is on the GPU.
struct Expected { uint32_t instances = 0; };
static float BruteForce(F3 o, F3 d, const bvh::BvhDeviceData& data, const bvh::PoolRegion& region, uint32_t* count) {
  std::lock_guard lock(bvh::g_pool.mutex);
  float best = 1e30f; bool any = false; uint32_t n = 0;
  for (const auto& inst : bvh::g_pool.instances) {
    if (!bvh::PoolInstanceInRegion(inst, region)) continue;
    const auto& mesh = bvh::g_pool.meshes[inst.mesh_id];
    const auto slot = data.slot_by_uid.find(mesh.uid);
    if (slot == data.slot_by_uid.end() || slot->second >= data.descriptor_count) continue;
    ++n;
    for (size_t t = 0; t + 2 < mesh.indices.size(); t += 3) {
      if (mesh.indices[t] >= mesh.positions.size() || mesh.indices[t + 1] >= mesh.positions.size()
          || mesh.indices[t + 2] >= mesh.positions.size()) {
        continue;  // like BuildMeshBlas: such triangles are not in the BVH
      }
      F3 v[3];
      for (int k = 0; k < 3; ++k) {
        const auto& p = mesh.positions[mesh.indices[t + k]];
        float w[3]; falcom_world::TransformInst4x3RowDot(inst.matrix, p[0], p[1], p[2], w);
        v[k] = {w[0], w[1], w[2]};
      }
      float tt;
      if (Tri(o, d, v[0], v[1], v[2], &tt) && tt >= 0.001f && tt < best) { best = tt; any = true; }
    }
  }
  if (count) *count = n;
  return any ? best : -1.f;
}

// Rays at triangle centroids of random region instances, plus random rays.
static int CompareTraces(const Device& dev, const bvh::BvhDeviceData& data, const char* label, int rays, uint32_t seed) {
  const bvh::PoolRegion region = bvh::CurrentPoolRegion();
  const TraceInputs in = Inputs(dev, data);
  std::mt19937 rng(seed);
  std::uniform_real_distribution<float> uni(-1.f, 1.f);
  float cam[3]; bvh::PoolCameraPosition(cam);
  const F3 origin = {cam[0], cam[1] + 30.f, cam[2]};
  int mismatches = 0, hits = 0;
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
  for (int i = 0; i < rays; ++i) {
    F3 dir;
    if (!targets.empty() && i % 2 == 0) {
      dir = targets[rng() % targets.size()] - origin;
    } else {
      dir = {uni(rng), uni(rng) - 0.3f, uni(rng)};
    }
    const float len = std::sqrt(Dot(dir, dir));
    if (len < 1e-6f) continue;
    dir = dir * (1.f / len);
    uint32_t instance;
    const float t_bvh = TraceWorld(in, origin, dir, c, &instance);
    const float t_ref = BruteForce(origin, dir, data, region, nullptr);
    const bool agree = (t_bvh < 0.f && t_ref < 0.f)
                       || (t_bvh >= 0.f && t_ref >= 0.f && std::fabs(t_bvh - t_ref) <= 1e-3f * std::max(1.f, t_ref));
    if (t_ref >= 0.f) ++hits;
    if (!agree) {
      if (mismatches < 3) std::printf("  %s ray %d: bvh %.5f ref %.5f\n", label, i, t_bvh, t_ref);
      ++mismatches;
    }
  }
  std::printf("%s: %d rays, %d hits, %d mismatches, invalid %u overflow %u\n", label, rays, hits, mismatches, c.invalid, c.overflow);
  CHECK(c.invalid == 0 && c.overflow == 0, "%s trace counters", label);
  CHECK(hits > rays / 4, "%s enough hits (%d)", label, hits);
  return mismatches;
}

// Brute force with the game camera fade, from the pool's own rule
// (GetPoolCameraVisibility, CameraFadeApplies, camera_fade.hpp).
static float BruteForceFade(F3 o, F3 d, const bvh::BvhDeviceData& data, const bvh::PoolRegion& region, Fade fade, uint32_t* faded) {
  std::lock_guard lock(bvh::g_pool.mutex);
  float best = 1e30f; bool any = false; bool best_faded = false; float faded_t = 3.0e38f;
  for (const auto& inst : bvh::g_pool.instances) {
    if (!bvh::PoolInstanceInRegion(inst, region)) continue;
    const auto& mesh = bvh::g_pool.meshes[inst.mesh_id];
    const auto slot = data.slot_by_uid.find(mesh.uid);
    if (slot == data.slot_by_uid.end() || slot->second >= data.descriptor_count) continue;
    const bvh::PoolCameraVisibility vis = bvh::GetPoolCameraVisibility(inst);
    float lo = 0.f, hi = bvh::kCameraFadeNever;
    if (!vis.camera_seen) {
      lo = bvh::kCameraFadeNever;
      hi = -1.f;
    } else if (bvh::CameraFadeApplies(vis)) {
      bvh::CameraFadeInterval(vis.inputs.param[0], vis.inputs.param[1], fade.floor_value, &lo, &hi);
    }
    for (size_t t = 0; t + 2 < mesh.indices.size(); t += 3) {
      if (mesh.indices[t] >= mesh.positions.size() || mesh.indices[t + 1] >= mesh.positions.size()
          || mesh.indices[t + 2] >= mesh.positions.size()) continue;
      F3 v[3];
      for (int k = 0; k < 3; ++k) {
        const auto& p = mesh.positions[mesh.indices[t + k]];
        float w[3]; falcom_world::TransformInst4x3RowDot(inst.matrix, p[0], p[1], p[2], w);
        v[k] = {w[0], w[1], w[2]};
      }
      float tt;
      if (!Tri(o, d, v[0], v[1], v[2], &tt) || tt < 0.001f) continue;
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
  for (int i = 0; i < rays; ++i) {
    F3 dir = (i % 3 != 0 && !targets.empty()) ? targets[rng() % targets.size()] - origin + F3{uni(rng), uni(rng), uni(rng)}
                                              : F3{uni(rng), uni(rng) - 0.3f, uni(rng)};
    const float len = std::sqrt(Dot(dir, dir));
    if (len < 1e-6f) continue;
    dir = dir * (1.f / len);
    uint32_t instance, faded_bvh = 0u, faded_ref = 0u;
    const float t_bvh = TraceWorld(in, origin, dir, c, &instance, fade, &faded_bvh);
    const float t_ref = BruteForceFade(origin, dir, data, region, fade, &faded_ref);
    const bool agree = ((t_bvh < 0.f && t_ref < 0.f)
                        || (t_bvh >= 0.f && t_ref >= 0.f && std::fabs(t_bvh - t_ref) <= 1e-3f * std::max(1.f, t_ref)))
                       && faded_bvh == faded_ref;
    *faded_rays += faded_ref;
    if (!agree) {
      if (mismatches < 3) std::printf("  %s ray %d: bvh %.5f/%u ref %.5f/%u\n", label, i, t_bvh, faded_bvh, t_ref, faded_ref);
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

static uint32_t ExpectedRegionInstances(const bvh::BvhDeviceData& data) {
  float d[3] = {0, 1, 0};
  uint32_t n = 0;
  BruteForce({0, 0, 0}, {d[0], d[1], d[2]}, data, bvh::CurrentPoolRegion(), &n);
  return n;
}

int main() {
  Device dev; CmdList cl; cl.dev = &dev; Queue queue; queue.dev = &dev; queue.cl = &cl;
  bvh::g_pool.region_size = 512.f;
  SetCamera(10.f, 5.f, 10.f);
  auto& frame = falcom_world::g_state.frame;
  frame.store(1000u);
  auto present = [&]() { frame.fetch_add(1u); bvh::UpdateLiveBvh(&dev, &queue); };
  bvh::BvhDeviceData* data = bvh::GetBvhDeviceData(&dev);

  // Meshes in pool order: A 180k triangles, B 30k, C 20k (each over the budget alone), D 12, E 28.8k.
  const uint64_t kA = 0xA, kB = 0xB, kC = 0xC, kD = 0xD, kE = 0xE;
  AddMesh(kA, Grid(300, 300, 40.f, 2.f));
  AddMesh(kB, Grid(150, 100, 20.f, 1.f));
  AddMesh(kC, Grid(100, 100, 10.f, 1.f));
  AddMesh(kD, BoxMesh(1.f));
  AddMesh(kE, Grid(120, 120, 15.f, 3.f));
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
  CHECK(data->live.tlas_instances == ExpectedRegionInstances(*data), "matches brute-force set");
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
    bvh::g_live_bvh.check_requested.store(true);
    present();
    std::printf("check: %s\n", data->live.gpu_result.c_str());
    CHECK(data->live.gpu_checked && data->live.gpu_ok, "GPU check");
    CHECK(renodx::utils::scene::g_readback_calls > calls, "check reads back");
  }

  // 2. A new instance: TLAS rebuilt once the interval has passed.
  AddInstance(kD, 5, 30, 5, 3.f);
  present();
  CHECK(std::string(data->live.tlas_reason) == "pool changed" && data->live.tlas_instances == 36u, "new instance (%s %u)", data->live.tlas_reason, data->live.tlas_instances);

  // 3. Region moves with the camera.
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
  bvh::g_live_bvh.check_requested.store(true);
  present();
  CHECK(data->live.gpu_ok, "GPU check after retirement: %s", data->live.gpu_result.c_str());

  // 5. Retire A: retired data passes half of the store -> reset and restream.
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
    bvh::g_live_bvh.check_requested.store(true);
    present();
    CHECK(data->live.gpu_checked && data->live.gpu_ok, "GPU check with visibility (%s)", data->live.gpu_result.c_str());
    RetireMesh(kF);
    RetireMesh(kG);
    for (int i = 0; i < 9; ++i) present();
    CHECK(data->live.tlas_near_fade == 0u, "near-fade instances gone with their meshes (%u)", data->live.tlas_near_fade);
  }

  // 9. Rebuild on request, also inside an upload retry window.
  data->upload_retry_frame = frame.load() + 100u;
  bvh::g_live_bvh.reset_requested.store(true);
  present();
  CHECK(data->live.resets == 2u && data->live.last_reset_reason == "requested", "requested reset");
  CHECK(data->upload_retry_frame == 0u, "retry window cleared");
  for (int i = 0; i < 12; ++i) present();
  CHECK(data->live.resident_meshes == 6u && data->bvh_ready, "restreamed after request (%u)", data->live.resident_meshes);
  CHECK(CompareTraces(dev, *data, "rebuilt", 300, 5) == 0, "trace agrees after rebuild");
  bvh::g_live_bvh.check_requested.store(true);
  present();
  CHECK(data->live.gpu_ok, "GPU check after rebuild: %s", data->live.gpu_result.c_str());

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
  bvh::ResetWorldPool();
  present();
  CHECK(data->live.resets == 3u && data->live.last_reset_reason == "no live mesh left", "pool reset (%s)", data->live.last_reset_reason.c_str());
  for (int i = 0; i < 9; ++i) present();
  CHECK(!data->bvh_ready && data->live.tlas_instances == 0u, "empty BVH");

  CHECK(g_under_lock == 0, "graphics calls under the pool lock: %d", g_under_lock);
  std::printf("whole-buffer updates %d, copies between different strides %d\n", dev.whole_buffer_updates, dev.structured_mismatch_copies);
  CHECK(dev.structured_mismatch_copies == 0, "GPU copies keep the element stride");

  // 12. Device teardown releases every buffer and view.
  bvh::DestroyBvhDeviceData(&dev);
  CHECK(dev.res.empty() && dev.views.empty(), "leaks: %zu resources, %zu views", dev.res.size(), dev.views.size());

  std::printf(g_failures == 0 ? "PASS (0 failures)\n" : "FAILED (%d failures)\n", g_failures);
  return g_failures == 0 ? 0 : 1;
}
