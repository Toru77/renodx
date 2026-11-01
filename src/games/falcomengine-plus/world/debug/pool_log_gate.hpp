#pragma once

// Log gate for the pool and deform subsystems (bvh/bvh_pool.hpp, bvh/deform_live.hpp).
// kEnabled = false silences every log line from those files. The argument
// expressions are still evaluated; flip kEnabled to true to log again.

#include <utility>

#include "../../../../utils/log.hpp"

namespace falcom_world::pool_log {

inline constexpr bool kEnabled = false;

template <typename... Args>
inline void i(Args&&... args) {
  if constexpr (kEnabled) renodx::utils::log::i(std::forward<Args>(args)...);
}

template <typename... Args>
inline void w(Args&&... args) {
  if constexpr (kEnabled) renodx::utils::log::w(std::forward<Args>(args)...);
}

}  // namespace falcom_world::pool_log
