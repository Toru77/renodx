#pragma once
// Stage timing for harness tests. HStage prints a stage line and records how long
// the previous stage ran; HProgress prints without recording. An atexit hook prints
// the three slowest stages and one "TIMING test=..." line that run_win_one.ps1 parses.
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace harness_timer {

struct StageRecord { std::string name; double seconds; };

inline const auto g_start = std::chrono::steady_clock::now();
inline double g_last = 0.0;
inline std::string g_pending = "startup";
inline std::vector<StageRecord> g_stages;

inline double Now() { return std::chrono::duration<double>(std::chrono::steady_clock::now() - g_start).count(); }

inline void PrintTiming() {
  const double total = Now();
  if (total - g_last > 0.0) g_stages.push_back({g_pending, total - g_last});
  std::vector<StageRecord> sorted = g_stages;
  std::stable_sort(sorted.begin(), sorted.end(), [](const StageRecord& a, const StageRecord& b) { return a.seconds > b.seconds; });
  for (size_t i = 0; i < sorted.size() && i < 3; ++i) {
    std::printf("slow %zu: %.2fs \"%s\"\n", i + 1, sorted[i].seconds, sorted[i].name.c_str());
  }
  const std::string exe = std::filesystem::path(_pgmptr).filename().string();
  const StageRecord top = sorted.empty() ? StageRecord{"none", 0.0} : sorted[0];
  std::printf("TIMING test=%s total=%.2f slowest=%.2f \"%s\"\n", exe.c_str(), total, top.seconds, top.name.c_str());
}

inline const int g_timing_hook = (std::atexit(PrintTiming), 0);

}  // namespace harness_timer

inline void HStage(const char* fmt, ...) {
  using namespace harness_timer;
  const double now = Now();
  char name[512];
  va_list args; va_start(args, fmt); std::vsnprintf(name, sizeof(name), fmt, args); va_end(args);
  std::printf("[%7.1fs +%6.1fs] %s\n", now, now - g_last, name);
  if (now - g_last > 0.0) g_stages.push_back({g_pending, now - g_last});
  g_pending = name;
  g_last = now;
}

inline void HProgress(const char* fmt, ...) {
  using namespace harness_timer;
  const double now = Now();
  char name[512];
  va_list args; va_start(args, fmt); std::vsnprintf(name, sizeof(name), fmt, args); va_end(args);
  std::printf("[%7.1fs +%6.1fs] %s\n", now, now - g_last, name);
}
