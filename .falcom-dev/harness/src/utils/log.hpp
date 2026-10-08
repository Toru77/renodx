#pragma once
// Harness stub of src/utils/log.hpp: same formatting helpers as the real
// header; lines are recorded in g_lines (level char, text) instead of going
// to ReShade.log.
#include <cstdint>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <include/reshade.hpp>

namespace renodx::utils::log {

template <typename T>
inline std::string AsHex(T value) {
  std::ostringstream oss;
  oss << "0x" << std::hex << std::setw(sizeof(T) * 2) << std::setfill('0') << value << std::dec;
  return oss.str();
}

template <typename T>
inline std::string AsPtr(T value) {
  std::ostringstream oss;
  oss << "0x" << std::hex << std::setw(sizeof(uint64_t) * 2) << std::setfill('0') << reinterpret_cast<uint64_t>(value) << std::dec;
  return oss.str();
}

inline std::string AsPtr(uint64_t value) {
  std::ostringstream oss;
  oss << "0x" << std::hex << std::setw(sizeof(uint64_t) * 2) << std::setfill('0') << value << std::dec;
  return oss.str();
}

inline std::string AsPtr(uint32_t value) {
  std::ostringstream oss;
  oss << "0x" << std::hex << std::setw(sizeof(uint32_t) * 2) << std::setfill('0') << value << std::dec;
  return oss.str();
}

template <typename... Args>
std::string BuildString(Args&&... args) {
  std::stringstream s;
  (s << ... << std::forward<Args>(args));
  return s.str();
}

inline std::vector<std::pair<char, std::string>> g_lines;
inline std::mutex g_lines_mutex;

inline void Record(char level, std::string line) {
  std::lock_guard<std::mutex> lock(g_lines_mutex);
  g_lines.emplace_back(level, std::move(line));
}

template <typename... Args>
void i(Args&&... args) { Record('i', BuildString(std::forward<Args>(args)...)); }
template <typename... Args>
void d(Args&&... args) { Record('d', BuildString(std::forward<Args>(args)...)); }
template <typename... Args>
void e(Args&&... args) { Record('e', BuildString(std::forward<Args>(args)...)); }
template <typename... Args>
void w(Args&&... args) { Record('w', BuildString(std::forward<Args>(args)...)); }

}  // namespace renodx::utils::log
