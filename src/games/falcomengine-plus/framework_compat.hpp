/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <utility>

#include "../../utils/data.hpp"

namespace falcom_framework_compat {
static bool isolate_pipeline_layout = false;
}

namespace renodx::utils::platform {

// This intercepts only the process-shared slot lookup used by cross_addon.hpp.
template <typename T>
inline ProcessSharedSlot<T> OpenFalcomProcessSharedSlot(const GUID& guid, auto&& initialize) {
  static constexpr GUID PIPELINE_LAYOUT = {0x080a74f2, 0x9a2a, 0x4af6, {0xbb, 0x2c, 0x8d, 0x08, 0x3e, 0x0a, 0x35, 0x4d}};
  static constexpr GUID PRIVATE_PIPELINE_LAYOUT = {0x65b574c4, 0x61b4, 0x4ce3, {0x89, 0xca, 0x71, 0xb5, 0xd6, 0x91, 0xc3, 0xe1}};
  const auto& slot_guid = (falcom_framework_compat::isolate_pipeline_layout && guid == PIPELINE_LAYOUT
                               ? PRIVATE_PIPELINE_LAYOUT
                               : guid);
  return OpenProcessSharedSlot<T>(slot_guid, std::forward<decltype(initialize)>(initialize));
}

}  // namespace renodx::utils::platform

// Keep the framework unchanged; this translation unit selects the slot above.
#define OpenProcessSharedSlot OpenFalcomProcessSharedSlot
#include "../../utils/cross_addon.hpp"
#undef OpenProcessSharedSlot
