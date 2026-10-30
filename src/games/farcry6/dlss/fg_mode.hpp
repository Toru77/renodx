/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 */
#pragma once

namespace farcry6_dlss::frame_generation {

// DLSS-G options are applied by the next successful present, not by the call
// to slDLSSGSetOptions. An unsuccessful present leaves the mode uncertain.
struct ModeState {
  enum class Mode { off, on, unknown };

  Mode applied = Mode::off;
  Mode pending = Mode::unknown;
  bool has_pending = false;

  void OptionsAccepted(Mode requested) {
    pending = requested;
    has_pending = true;
  }

  void PresentCompleted(bool success) {
    if (!has_pending) return;
    applied = (success ? pending : Mode::unknown);
    pending = Mode::unknown;
    has_pending = false;
  }

  bool ReadyForDisplayChange() const {
    return applied == Mode::off && !has_pending;
  }
};

}  // namespace farcry6_dlss::frame_generation
