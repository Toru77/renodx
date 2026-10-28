/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 */
#include <cassert>

#include "../fg_mode.hpp"

int main() {
  using farcry6_dlss::frame_generation::ModeState;
  ModeState state;
  assert(state.ReadyForDisplayChange());
  state.OptionsAccepted(ModeState::Mode::on);
  assert(!state.ReadyForDisplayChange());
  state.PresentCompleted(true);
  assert(state.applied == ModeState::Mode::on && !state.ReadyForDisplayChange());
  state.OptionsAccepted(ModeState::Mode::off);
  assert(!state.ReadyForDisplayChange());
  state.PresentCompleted(true);
  assert(state.ReadyForDisplayChange());
  state.OptionsAccepted(ModeState::Mode::on);
  state.PresentCompleted(false);
  assert(state.applied == ModeState::Mode::unknown && !state.ReadyForDisplayChange());
  state.OptionsAccepted(ModeState::Mode::off);
  state.PresentCompleted(false);
  assert(!state.ReadyForDisplayChange());
  state.OptionsAccepted(ModeState::Mode::off);
  state.PresentCompleted(true);
  assert(state.ReadyForDisplayChange());
}
