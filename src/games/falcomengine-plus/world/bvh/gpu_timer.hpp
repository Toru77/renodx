#pragma once

// GPU time of one pass from D3D11 timestamp queries, read without waiting.
//
// Each Begin/End pair uses one of kGpuTimerSlots query sets (a disjoint query
// around two timestamps); a set is read a few frames later with
// D3D11_ASYNC_GETDATA_DONOTFLUSH, so nothing waits for the GPU. A frame is not
// measured while every set is still in flight. The native immediate context
// is used directly: queries do not touch pipeline state, and no pool lock is
// involved. Other APIs (and the native harness) get a timer that never
// measures.

#include <cstdint>

#if defined(_WIN32)
#include <d3d11.h>
#endif

#include <include/reshade.hpp>

namespace falcom_world::bvh {

inline constexpr uint32_t kGpuTimerSlots = 4u;

struct GpuTimer {
#if defined(_WIN32)
  struct Slot {
    ID3D11Query* disjoint = nullptr;
    ID3D11Query* begin = nullptr;
    ID3D11Query* end = nullptr;
    bool pending = false;
    uint64_t serial = 0u;
  };
  Slot slots[kGpuTimerSlots];
  uint32_t next = 0u;
  int32_t open = -1;  // slot between Begin and End
  uint64_t serial = 0u;
  uint64_t last_serial = 0u;
  bool created = false;
  bool failed = false;
#endif
  float last_ms = -1.f;  // newest completed measurement; negative: none yet
  uint32_t samples = 0u;
  uint32_t skipped = 0u;  // frames not measured (every set in flight, or a disjoint interval)
};

#if defined(_WIN32)

inline ID3D11DeviceContext* GpuTimerContext(reshade::api::command_list* cmd_list) {
  if (cmd_list == nullptr) return nullptr;
  return reinterpret_cast<ID3D11DeviceContext*>(static_cast<uintptr_t>(cmd_list->get_native()));  // NOLINT(performance-no-int-to-ptr)
}

inline void DestroyGpuTimer(GpuTimer* timer) {
  if (timer == nullptr) return;
  for (auto& slot : timer->slots) {
    if (slot.disjoint != nullptr) slot.disjoint->Release();
    if (slot.begin != nullptr) slot.begin->Release();
    if (slot.end != nullptr) slot.end->Release();
    slot = {};
  }
  timer->next = 0u;
  timer->open = -1;
  timer->created = false;
  timer->failed = false;
  timer->last_ms = -1.f;
}

inline bool CreateGpuTimer(reshade::api::device* device, GpuTimer* timer) {
  if (timer->created) return true;
  if (timer->failed || device == nullptr || device->get_api() != reshade::api::device_api::d3d11) return false;
  auto* native = reinterpret_cast<ID3D11Device*>(static_cast<uintptr_t>(device->get_native()));  // NOLINT(performance-no-int-to-ptr)
  if (native == nullptr) {
    timer->failed = true;
    return false;
  }
  D3D11_QUERY_DESC disjoint_desc = {D3D11_QUERY_TIMESTAMP_DISJOINT, 0u};
  D3D11_QUERY_DESC timestamp_desc = {D3D11_QUERY_TIMESTAMP, 0u};
  for (auto& slot : timer->slots) {
    if (FAILED(native->CreateQuery(&disjoint_desc, &slot.disjoint))
        || FAILED(native->CreateQuery(&timestamp_desc, &slot.begin))
        || FAILED(native->CreateQuery(&timestamp_desc, &slot.end))) {
      DestroyGpuTimer(timer);
      timer->failed = true;
      return false;
    }
  }
  timer->created = true;
  return true;
}

// Reads every finished set; keeps the newest result.
inline void PollGpuTimer(ID3D11DeviceContext* context, GpuTimer* timer) {
  for (auto& slot : timer->slots) {
    if (!slot.pending) continue;
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint = {};
    if (context->GetData(slot.disjoint, &disjoint, sizeof(disjoint), D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK) continue;
    slot.pending = false;
    UINT64 begin = 0u;
    UINT64 end = 0u;
    if (disjoint.Disjoint || disjoint.Frequency == 0u
        || context->GetData(slot.begin, &begin, sizeof(begin), D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK
        || context->GetData(slot.end, &end, sizeof(end), D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK || end < begin) {
      timer->skipped += 1u;
      continue;
    }
    if (slot.serial < timer->last_serial) continue;
    timer->last_serial = slot.serial;
    timer->last_ms = static_cast<float>(static_cast<double>(end - begin) * 1000.0 / static_cast<double>(disjoint.Frequency));
    timer->samples += 1u;
  }
}

inline void BeginGpuTimer(reshade::api::device* device, reshade::api::command_list* cmd_list, GpuTimer* timer) {
  if (timer == nullptr || timer->open >= 0 || !CreateGpuTimer(device, timer)) return;
  ID3D11DeviceContext* context = GpuTimerContext(cmd_list);
  if (context == nullptr) return;
  PollGpuTimer(context, timer);
  auto& slot = timer->slots[timer->next];
  if (slot.pending) {
    timer->skipped += 1u;
    return;
  }
  context->Begin(slot.disjoint);
  context->End(slot.begin);
  timer->open = static_cast<int32_t>(timer->next);
}

inline void EndGpuTimer(reshade::api::command_list* cmd_list, GpuTimer* timer) {
  if (timer == nullptr || timer->open < 0) return;
  ID3D11DeviceContext* context = GpuTimerContext(cmd_list);
  auto& slot = timer->slots[timer->open];
  timer->open = -1;
  if (context == nullptr) return;
  context->End(slot.end);
  context->End(slot.disjoint);
  slot.pending = true;
  slot.serial = ++timer->serial;
  timer->next = (timer->next + 1u) % kGpuTimerSlots;
}

#else

inline void DestroyGpuTimer(GpuTimer* timer) {
  if (timer != nullptr) timer->last_ms = -1.f;
}
inline void BeginGpuTimer(reshade::api::device*, reshade::api::command_list*, GpuTimer*) {}
inline void EndGpuTimer(reshade::api::command_list*, GpuTimer*) {}

#endif

}  // namespace falcom_world::bvh
