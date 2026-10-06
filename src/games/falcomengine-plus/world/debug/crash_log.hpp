#pragma once

// Crash log (diagnostic, BVH panel "Log crashes", off by default).
//
// While on, every report goes to ReShade.log with the thread, the world-pool
// stage the thread was in (debug/pool_stage.hpp), process memory, the D3D11
// device-removed state and the call stack as module+offset per frame.
// Offsets inside this add-on are resolved offline with the build's PDB; the
// line written when the log is turned on records the PDB signature.
//
// Reported, without changing what happens next (the game still crashes):
//   - fatal SEH exceptions (access violation, illegal instruction, ...) and
//     thrown C++ exceptions, through a vectored exception handler;
//   - abort(), invalid CRT parameters and pure virtual calls in the shared
//     C runtime (ucrtbase.dll, used by the game), through its SIGABRT,
//     invalid-parameter and purecall handlers. Those end in a fast fail
//     (0xC0000409) that no exception handler sees. Each handler logs and then
//     does what the runtime would have done without it.
//   - the device becoming removed, checked once per present.
//
// Vectored handlers also see first-chance exceptions that something handles
// later, so each faulting address (or C++ throw stack) is reported once, for
// at most kCrashMaxReports reports per enable.

#include <Windows.h>
#include <d3d11.h>
#include <psapi.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>

#include <include/reshade.hpp>

#include "../../../../utils/log.hpp"
#include "../world_state.hpp"
#include "./pool_stage.hpp"

namespace falcom_world {

inline constexpr uint32_t kCrashMaxReports = 64u;
inline constexpr uint32_t kCrashMaxFrames = 48u;
inline constexpr uint32_t kCrashCppHashFrames = 10u;
inline constexpr uint32_t kCrashMemoryLogFrames = 600u;  // periodic memory line while on
inline constexpr DWORD kCrashCppExceptionCode = 0xE06D7363u;

// Shared C runtime entry points (ucrtbase.dll).
using CrtSignalHandler = void(__cdecl*)(int);
using CrtSignalFn = CrtSignalHandler(__cdecl*)(int, CrtSignalHandler);
using CrtInvalidParameterHandler = void(__cdecl*)(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t);
using CrtSetInvalidParameterFn = CrtInvalidParameterHandler(__cdecl*)(CrtInvalidParameterHandler);
using CrtPurecallHandler = void(__cdecl*)();
using CrtSetPurecallFn = CrtPurecallHandler(__cdecl*)(CrtPurecallHandler);
using CrtInvokeWatsonFn = void(__cdecl*)(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t);
using ProcessMemoryInfoFn = BOOL(WINAPI*)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);

inline constexpr int kCrtSigAbrt = 22;

inline CrtSignalHandler CrtSignalValue(intptr_t value) { return reinterpret_cast<CrtSignalHandler>(value); }

struct CrashCrtHooks {
  CrtSignalFn signal = nullptr;
  CrtSetInvalidParameterFn set_invalid_parameter = nullptr;
  CrtSetPurecallFn set_purecall = nullptr;
  CrtInvokeWatsonFn invoke_watson = nullptr;
  CrtSignalHandler previous_abort = nullptr;
  CrtInvalidParameterHandler previous_invalid_parameter = nullptr;
  CrtPurecallHandler previous_purecall = nullptr;
  bool abort_installed = false;
  bool invalid_parameter_installed = false;
  bool purecall_installed = false;
};

struct CrashLogState {
  std::mutex mutex;  // install / remove
  void* handler = nullptr;
  CrashCrtHooks crt;
  std::atomic_bool enabled{false};
  std::atomic_uint32_t reports{0u};
  std::array<std::atomic_uint64_t, kCrashMaxReports> keys = {};  // reported addresses / stack hashes
  std::atomic_uint64_t native_device{0u};                        // ID3D11Device*, set at present
  std::atomic<long> logged_removed_reason{S_OK};
  std::atomic_uint32_t presents{0u};
};

inline CrashLogState g_crash_log;

inline thread_local bool t_crash_in_handler = false;

// True the first time a key is reported; false when it was reported before
// or the table is full.
inline bool CrashFirstReport(uint64_t key) {
  for (auto& slot : g_crash_log.keys) {
    uint64_t current = slot.load();
    if (current == key) return false;
    if (current != 0u) continue;
    if (slot.compare_exchange_strong(current, key)) return true;
    if (current == key) return false;
  }
  return false;
}

inline const char* CrashExceptionName(DWORD code) {
  switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:      return "access violation";
    case EXCEPTION_ILLEGAL_INSTRUCTION:   return "illegal instruction";
    case EXCEPTION_PRIV_INSTRUCTION:      return "privileged instruction";
    case EXCEPTION_IN_PAGE_ERROR:         return "in-page error";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:    return "integer divide by zero";
    case EXCEPTION_STACK_OVERFLOW:        return "stack overflow";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "array bounds exceeded";
    case EXCEPTION_DATATYPE_MISALIGNMENT: return "datatype misalignment";
    case 0xC0000374u:                     return "heap corruption";
    default:                              return nullptr;
  }
}

inline const char* CrashRemovedReasonName(HRESULT hr) {
  switch (static_cast<uint32_t>(hr)) {
    case 0x887A0005u: return "DXGI_ERROR_DEVICE_REMOVED";
    case 0x887A0006u: return "DXGI_ERROR_DEVICE_HUNG";
    case 0x887A0007u: return "DXGI_ERROR_DEVICE_RESET";
    case 0x887A0020u: return "DXGI_ERROR_DRIVER_INTERNAL_ERROR";
    case 0x887A0001u: return "DXGI_ERROR_INVALID_CALL";
    case 0x8007000Eu: return "E_OUTOFMEMORY";
    default:          return "other";
  }
}

template <typename T>
inline bool CrashSafeRead(uint64_t address, T* out) {
  SIZE_T read = 0u;
  return address != 0u
         && ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(address), out, sizeof(T), &read)
         && read == sizeof(T);
}

// "module.dll+0x1234" for an address inside a loaded module, else the raw address.
inline void FormatCrashAddress(uint64_t address, char* out, size_t size) {
  HMODULE module = nullptr;
  if (address != 0u
      && GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCSTR>(address), &module)
      && module != nullptr) {
    char path[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameA(module, path, MAX_PATH);
    const char* name = path;
    for (DWORD i = 0; i < length; ++i) {
      if (path[i] == '\\' || path[i] == '/') name = path + i + 1;
    }
    std::snprintf(out, size, "%s+0x%llx", name,
                  static_cast<unsigned long long>(address - reinterpret_cast<uint64_t>(module)));
    return;
  }
  std::snprintf(out, size, "0x%016llx", static_cast<unsigned long long>(address));
}

struct CrashFrames {
  uint64_t pc[kCrashMaxFrames] = {};
  uint32_t count = 0u;
};

// Walks the stack from a context with the x64 unwind tables.
inline void CollectCrashFrames(const CONTEXT& start, CrashFrames* frames) {
  CONTEXT context = start;
  frames->count = 0u;
  while (frames->count < kCrashMaxFrames) {
    const DWORD64 pc = context.Rip;
    if (pc == 0u) break;
    frames->pc[frames->count++] = pc;

    DWORD64 image_base = 0u;
    PRUNTIME_FUNCTION function = RtlLookupFunctionEntry(pc, &image_base, nullptr);
    if (function == nullptr) {
      // Leaf function: the return address is at the stack pointer.
      DWORD64 return_address = 0u;
      if (!CrashSafeRead(context.Rsp, &return_address)) break;
      context.Rip = return_address;
      context.Rsp += sizeof(DWORD64);
    } else {
      PVOID handler_data = nullptr;
      DWORD64 establisher_frame = 0u;
      RtlVirtualUnwind(UNW_FLAG_NHANDLER, image_base, pc, function, &context, &handler_data, &establisher_frame,
                       nullptr);
    }
  }
}

inline uint64_t CrashFramesHash(const CrashFrames& frames, uint32_t depth) {
  uint64_t hash = 1469598103934665603ull;
  for (uint32_t i = 0; i < frames.count && i < depth; ++i) {
    hash ^= frames.pc[i];
    hash *= 1099511628211ull;
  }
  return hash | 1u;
}

inline void LogCrashFrames(const CrashFrames& frames) {
  for (uint32_t i = 0; i < frames.count; ++i) {
    char where[MAX_PATH + 32] = {};
    FormatCrashAddress(frames.pc[i], where, sizeof(where));
    char line[MAX_PATH + 64] = {};
    std::snprintf(line, sizeof(line), "falcom_world::crash:   #%02u %s", i, where);
    renodx::utils::log::e(line);
  }
}

inline void FormatCrashMemory(char* out, size_t size) {
  std::snprintf(out, size, "memory unknown");
  HMODULE kernel = GetModuleHandleA("kernel32.dll");
  if (kernel == nullptr) return;
  const auto query = reinterpret_cast<ProcessMemoryInfoFn>(
      reinterpret_cast<void*>(GetProcAddress(kernel, "K32GetProcessMemoryInfo")));
  if (query == nullptr) return;
  PROCESS_MEMORY_COUNTERS_EX counters = {};
  counters.cb = sizeof(counters);
  if (!query(GetCurrentProcess(), reinterpret_cast<PPROCESS_MEMORY_COUNTERS>(&counters), sizeof(counters))) return;
  std::snprintf(out, size, "private %llu MB, working set %llu MB",
                static_cast<unsigned long long>(counters.PrivateUsage >> 20u),
                static_cast<unsigned long long>(counters.WorkingSetSize >> 20u));
}

// Writes the header and context lines of one report, then the stack.
inline void LogCrashReport(const char* description, const CrashFrames& frames) {
  const uint32_t report = g_crash_log.reports.fetch_add(1u) + 1u;
  const char* stage = CurrentPoolStage();
  char header[3 * MAX_PATH] = {};
  std::snprintf(header, sizeof(header), "falcom_world::crash: report %u: %s | thread %lu, frame %u, pool stage: %s",
                report, description, static_cast<unsigned long>(GetCurrentThreadId()), g_state.frame.load(),
                stage != nullptr ? stage : "none");
  renodx::utils::log::e(header);

  char memory[128] = {};
  FormatCrashMemory(memory, sizeof(memory));
  char state[256] = {};
  const uint64_t native = g_crash_log.native_device.load();
  if (native != 0u) {
    const HRESULT reason = reinterpret_cast<ID3D11Device*>(native)->GetDeviceRemovedReason();
    std::snprintf(state, sizeof(state), "falcom_world::crash:   %s | device %s (0x%08lX %s)", memory,
                  reason == S_OK ? "ok" : "removed", static_cast<unsigned long>(reason),
                  reason == S_OK ? "S_OK" : CrashRemovedReasonName(reason));
  } else {
    std::snprintf(state, sizeof(state), "falcom_world::crash:   %s | device unknown", memory);
  }
  renodx::utils::log::e(state);
  LogCrashFrames(frames);
}

// Name of the first catchable type of a thrown C++ exception, decorated
// (".?AVbad_alloc@std@@").
inline void CrashCppTypeName(const EXCEPTION_RECORD& record, char* out, size_t size) {
  std::snprintf(out, size, "unknown type");
  if (record.NumberParameters < 4u) return;
  const uint64_t throw_info = static_cast<uint64_t>(record.ExceptionInformation[2]);
  const uint64_t image_base = static_cast<uint64_t>(record.ExceptionInformation[3]);
  if (throw_info == 0u || image_base == 0u) return;
  int32_t catchable_array = 0;
  if (!CrashSafeRead(throw_info + 12u, &catchable_array) || catchable_array == 0) return;
  int32_t first_type = 0;
  if (!CrashSafeRead(image_base + static_cast<uint32_t>(catchable_array) + 4u, &first_type) || first_type == 0) return;
  int32_t descriptor = 0;
  if (!CrashSafeRead(image_base + static_cast<uint32_t>(first_type) + 4u, &descriptor) || descriptor == 0) return;
  char name[128] = {};
  SIZE_T read = 0u;
  if (!ReadProcessMemory(GetCurrentProcess(),
                         reinterpret_cast<LPCVOID>(image_base + static_cast<uint32_t>(descriptor) + 16u), name,
                         sizeof(name) - 1u, &read)
      || read == 0u) {
    return;
  }
  name[sizeof(name) - 1u] = '\0';
  std::snprintf(out, size, "%s", name);
}

inline LONG CALLBACK CrashLogHandler(EXCEPTION_POINTERS* info) {
  if (t_crash_in_handler || info == nullptr || info->ExceptionRecord == nullptr || info->ContextRecord == nullptr) {
    return EXCEPTION_CONTINUE_SEARCH;
  }
  if (!g_crash_log.enabled.load(std::memory_order_relaxed)) return EXCEPTION_CONTINUE_SEARCH;
  const EXCEPTION_RECORD& record = *info->ExceptionRecord;
  const bool cpp = record.ExceptionCode == kCrashCppExceptionCode;
  const char* name = cpp ? "C++ exception thrown" : CrashExceptionName(record.ExceptionCode);
  if (name == nullptr) return EXCEPTION_CONTINUE_SEARCH;
  if (!cpp && !CrashFirstReport(reinterpret_cast<uint64_t>(record.ExceptionAddress))) return EXCEPTION_CONTINUE_SEARCH;
  t_crash_in_handler = true;

  CrashFrames frames;
  CollectCrashFrames(*info->ContextRecord, &frames);
  // A C++ throw always starts in RaiseException: key it by its call stack.
  if (cpp && !CrashFirstReport(CrashFramesHash(frames, kCrashCppHashFrames))) {
    t_crash_in_handler = false;
    return EXCEPTION_CONTINUE_SEARCH;
  }

  char where[MAX_PATH + 32] = {};
  FormatCrashAddress(reinterpret_cast<uint64_t>(record.ExceptionAddress), where, sizeof(where));
  char detail[MAX_PATH + 160] = {};
  if (cpp) {
    char type[128] = {};
    CrashCppTypeName(record, type, sizeof(type));
    std::snprintf(detail, sizeof(detail), ", type %s (first chance: may be caught)", type);
  } else if (record.ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record.NumberParameters >= 2u) {
    const ULONG_PTR kind = record.ExceptionInformation[0];
    const char* verb = kind == 0u ? "reading" : (kind == 1u ? "writing" : (kind == 8u ? "executing" : "accessing"));
    char target[MAX_PATH + 32] = {};
    FormatCrashAddress(static_cast<uint64_t>(record.ExceptionInformation[1]), target, sizeof(target));
    std::snprintf(detail, sizeof(detail), ", %s %s", verb, target);
  }
  char description[3 * MAX_PATH] = {};
  std::snprintf(description, sizeof(description), "%s (0x%08lX) at %s%s", name,
                static_cast<unsigned long>(record.ExceptionCode), where, detail);
  LogCrashReport(description, frames);

  t_crash_in_handler = false;
  return EXCEPTION_CONTINUE_SEARCH;
}

// Logs a report for the current call stack (runtime handlers).
inline void LogCrashReportHere(const char* description) {
  if (t_crash_in_handler || !g_crash_log.enabled.load(std::memory_order_relaxed)) return;
  t_crash_in_handler = true;
  CONTEXT context = {};
  RtlCaptureContext(&context);
  CrashFrames frames;
  CollectCrashFrames(context, &frames);
  if (CrashFirstReport(CrashFramesHash(frames, kCrashMaxFrames))) LogCrashReport(description, frames);
  t_crash_in_handler = false;
}

// abort() raises SIGABRT when a handler is set; after the handler returns
// it fast-fails exactly as before.
inline void __cdecl CrashOnCrtAbort(int signal_number) {
  LogCrashReportHere("abort() called (SIGABRT in ucrtbase)");
  const CrtSignalHandler previous = g_crash_log.crt.previous_abort;
  if (previous != CrtSignalValue(0) && previous != CrtSignalValue(1) && previous != CrtSignalValue(-1)) {
    previous(signal_number);
  }
}

// Without a handler the runtime calls _invoke_watson (fast fail); with the
// game's own handler that one runs. Both stay the same here.
inline void __cdecl CrashOnCrtInvalidParameter(
    const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t reserved) {
  LogCrashReportHere("invalid parameter passed to a C runtime function (ucrtbase)");
  const CrtInvalidParameterHandler previous = g_crash_log.crt.previous_invalid_parameter;
  if (previous != nullptr) {
    previous(expression, function, file, line, reserved);
    return;
  }
  g_crash_log.crt.invoke_watson(expression, function, file, line, reserved);
}

// _purecall calls abort() after the handler returns.
inline void __cdecl CrashOnCrtPurecall() {
  LogCrashReportHere("pure virtual function call (ucrtbase)");
  const CrtPurecallHandler previous = g_crash_log.crt.previous_purecall;
  if (previous != nullptr) previous();
}

// Caller holds g_crash_log.mutex.
inline void InstallCrashCrtHooks() {
  CrashCrtHooks& crt = g_crash_log.crt;
  HMODULE ucrt = GetModuleHandleA("ucrtbase.dll");
  if (ucrt == nullptr) return;
  const auto find = [ucrt](const char* name) { return reinterpret_cast<void*>(GetProcAddress(ucrt, name)); };
  crt.signal = reinterpret_cast<CrtSignalFn>(find("signal"));
  crt.set_invalid_parameter = reinterpret_cast<CrtSetInvalidParameterFn>(find("_set_invalid_parameter_handler"));
  crt.set_purecall = reinterpret_cast<CrtSetPurecallFn>(find("_set_purecall_handler"));
  crt.invoke_watson = reinterpret_cast<CrtInvokeWatsonFn>(find("_invoke_watson"));

  if (crt.signal != nullptr) {
    const CrtSignalHandler previous = crt.signal(kCrtSigAbrt, CrashOnCrtAbort);
    if (previous != CrtSignalValue(-1)) {
      crt.previous_abort = previous;
      crt.abort_installed = true;
    }
  }
  if (crt.set_invalid_parameter != nullptr && crt.invoke_watson != nullptr) {
    crt.previous_invalid_parameter = crt.set_invalid_parameter(CrashOnCrtInvalidParameter);
    crt.invalid_parameter_installed = true;
  }
  if (crt.set_purecall != nullptr) {
    crt.previous_purecall = crt.set_purecall(CrashOnCrtPurecall);
    crt.purecall_installed = true;
  }
}

// Caller holds g_crash_log.mutex. Puts back what was there, unless something
// replaced our handler in the meantime (then that one stays).
inline void RemoveCrashCrtHooks() {
  CrashCrtHooks& crt = g_crash_log.crt;
  if (crt.abort_installed) {
    const CrtSignalHandler current = crt.signal(kCrtSigAbrt, crt.previous_abort);
    // raise() resets the handler to SIG_DFL before calling it.
    if (current != CrashOnCrtAbort && current != CrtSignalValue(0) && current != CrtSignalValue(-1)) {
      crt.signal(kCrtSigAbrt, current);
    }
  }
  if (crt.invalid_parameter_installed) {
    const CrtInvalidParameterHandler current = crt.set_invalid_parameter(crt.previous_invalid_parameter);
    if (current != CrashOnCrtInvalidParameter) crt.set_invalid_parameter(current);
  }
  if (crt.purecall_installed) {
    const CrtPurecallHandler current = crt.set_purecall(crt.previous_purecall);
    if (current != CrashOnCrtPurecall) crt.set_purecall(current);
  }
  crt = {};
}

// Logs this add-on's image and PDB signature, so crash offsets can be matched
// with the right PDB, and which runtime handlers are in place.
inline void LogCrashModuleInfo() {
  HMODULE module = nullptr;
  if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          reinterpret_cast<LPCSTR>(&CrashLogHandler), &module)
      || module == nullptr) {
    return;
  }
  const auto* base = reinterpret_cast<const uint8_t*>(module);
  const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
  if (dos->e_magic != IMAGE_DOS_SIGNATURE) return;
  const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
  if (nt->Signature != IMAGE_NT_SIGNATURE) return;

  char pdb[MAX_PATH + 96] = "no PDB record";
  const IMAGE_DATA_DIRECTORY& debug = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
  if (debug.VirtualAddress != 0u) {
    const auto* entries = reinterpret_cast<const IMAGE_DEBUG_DIRECTORY*>(base + debug.VirtualAddress);
    const size_t count = debug.Size / sizeof(IMAGE_DEBUG_DIRECTORY);
    for (size_t i = 0; i < count; ++i) {
      if (entries[i].Type != IMAGE_DEBUG_TYPE_CODEVIEW || entries[i].AddressOfRawData == 0u) continue;
      const uint8_t* codeview = base + entries[i].AddressOfRawData;
      if (std::memcmp(codeview, "RSDS", 4) != 0) continue;
      GUID guid = {};
      DWORD age = 0u;
      std::memcpy(&guid, codeview + 4, sizeof(guid));
      std::memcpy(&age, codeview + 4 + sizeof(guid), sizeof(age));
      const char* path = reinterpret_cast<const char*>(codeview + 8 + sizeof(guid));
      std::snprintf(pdb, sizeof(pdb), "pdb %08lX%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X age %lu %s",
                    static_cast<unsigned long>(guid.Data1), guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
                    guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7],
                    static_cast<unsigned long>(age), path);
      break;
    }
  }
  char path[MAX_PATH] = {};
  GetModuleFileNameA(module, path, MAX_PATH);
  char memory[128] = {};
  FormatCrashMemory(memory, sizeof(memory));
  const CrashCrtHooks& crt = g_crash_log.crt;
  char line[4 * MAX_PATH] = {};
  std::snprintf(line, sizeof(line),
                "falcom_world::crash: crash log on | %s base 0x%llx timestamp 0x%08lX | %s | ucrtbase handlers: "
                "abort %s, invalid parameter %s, purecall %s | %s",
                path, static_cast<unsigned long long>(reinterpret_cast<uint64_t>(module)),
                static_cast<unsigned long>(nt->FileHeader.TimeDateStamp), pdb, crt.abort_installed ? "on" : "off",
                crt.invalid_parameter_installed ? "on" : "off", crt.purecall_installed ? "on" : "off", memory);
  renodx::utils::log::i(line);
}

// Installs or removes all handlers. Must be off before the module unloads
// (world.hpp does on DLL_PROCESS_DETACH).
inline void SetCrashLogEnabled(bool enabled) {
  std::lock_guard<std::mutex> lock(g_crash_log.mutex);
  if (enabled && g_crash_log.handler == nullptr) {
    g_crash_log.reports.store(0u);
    for (auto& slot : g_crash_log.keys) slot.store(0u);
    g_crash_log.logged_removed_reason.store(S_OK);
    g_crash_log.handler = AddVectoredExceptionHandler(1u, CrashLogHandler);
    if (g_crash_log.handler == nullptr) return;
    InstallCrashCrtHooks();
    g_crash_log.enabled.store(true);
    LogCrashModuleInfo();
  } else if (!enabled && g_crash_log.handler != nullptr) {
    g_crash_log.enabled.store(false);
    RemoveCrashCrtHooks();
    RemoveVectoredExceptionHandler(g_crash_log.handler);
    g_crash_log.handler = nullptr;
  }
}

inline bool CrashLogEnabled() { return g_crash_log.enabled.load(std::memory_order_relaxed); }

// Present hook (registered while the module is attached; idle unless on):
// remembers the device for reports, logs a device removal once, and writes
// a memory line every kCrashMemoryLogFrames presents.
inline void OnCrashLogPresent(
    reshade::api::command_queue* queue,
    reshade::api::swapchain* swapchain,
    const reshade::api::rect* source_rect,
    const reshade::api::rect* dest_rect,
    uint32_t dirty_rect_count,
    const reshade::api::rect* dirty_rects) {
  (void)swapchain;
  (void)source_rect;
  (void)dest_rect;
  (void)dirty_rect_count;
  (void)dirty_rects;
  if (!CrashLogEnabled() || queue == nullptr) return;
  reshade::api::device* device = queue->get_device();
  if (device == nullptr || device->get_api() != reshade::api::device_api::d3d11) return;
  const uint64_t native = device->get_native();
  g_crash_log.native_device.store(native);
  if (native == 0u) return;

  const HRESULT reason = reinterpret_cast<ID3D11Device*>(native)->GetDeviceRemovedReason();
  if (reason != g_crash_log.logged_removed_reason.exchange(reason) && reason != S_OK) {
    char line[160] = {};
    std::snprintf(line, sizeof(line), "falcom_world::crash: device removed at frame %u: 0x%08lX %s",
                  g_state.frame.load(), static_cast<unsigned long>(reason), CrashRemovedReasonName(reason));
    renodx::utils::log::e(line);
  }
  if ((g_crash_log.presents.fetch_add(1u) % kCrashMemoryLogFrames) == 0u) {
    char memory[128] = {};
    FormatCrashMemory(memory, sizeof(memory));
    char line[192] = {};
    std::snprintf(line, sizeof(line), "falcom_world::crash: frame %u: %s", g_state.frame.load(), memory);
    renodx::utils::log::i(line);
  }
}

}  // namespace falcom_world
