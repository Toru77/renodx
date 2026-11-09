#pragma once

#include <cstdint>

namespace falcom_world::rtxdi {

enum class DiSubmissionGate { Ready,
                              WaitingInput,
                              StaleIdentity,
                              ConfigurationMismatch,
                              Busy };

struct DiSubmissionIdentity {
  uint32_t requested_frame = 0u;
  uint32_t transfer_frame = 0u;
  uint64_t requested_generation = 0u;
  uint64_t owner_generation = 0u;
  int32_t requested_scene_slot = -1;
  int32_t transfer_scene_slot = -1;
  int32_t requested_transfer_slot = -1;
  int32_t transfer_slot = -1;
  uint64_t requested_fence = 0u;
  uint64_t transfer_fence = 0u;
  uint32_t requested_width = 0u;
  uint32_t requested_height = 0u;
  uint32_t config_width = 0u;
  uint32_t config_height = 0u;
  bool local_requested = false;
  bool input_ready = false;
  bool transfer_available = false;
  bool reservoir_available = false;
};

inline DiSubmissionGate CheckDiSubmissionIdentity(const DiSubmissionIdentity& input) {
  if (!input.local_requested || !input.input_ready) return DiSubmissionGate::WaitingInput;
  if (input.requested_frame != input.transfer_frame || input.requested_generation == 0u
      || input.requested_generation != input.owner_generation || input.requested_scene_slot < 0
      || input.requested_scene_slot != input.transfer_scene_slot || input.requested_transfer_slot < 0
      || input.requested_transfer_slot != input.transfer_slot || input.requested_fence == 0u
      || input.requested_fence != input.transfer_fence) return DiSubmissionGate::StaleIdentity;
  if (input.requested_width == 0u || input.requested_height == 0u || input.requested_width != input.config_width
      || input.requested_height != input.config_height) return DiSubmissionGate::ConfigurationMismatch;
  if (!input.transfer_available || !input.reservoir_available) return DiSubmissionGate::Busy;
  return DiSubmissionGate::Ready;
}

enum class DiSignalDisposition { KeepSession,
                                 QuarantineSession };
inline DiSignalDisposition DiPostSubmitSignalDisposition(bool execution_submitted, bool signal_accepted) {
  return execution_submitted && !signal_accepted ? DiSignalDisposition::QuarantineSession : DiSignalDisposition::KeepSession;
}

enum class DiOutputGate { Ready,
                          NotStaged,
                          ProducerPending,
                          StaleIdentity,
                          ConfigurationMismatch };

struct DiOutputIdentity {
  uint32_t requested_frame = UINT32_MAX, output_frame = UINT32_MAX;
  uint64_t requested_owner_generation = 0u, output_owner_generation = 0u;
  uint64_t requested_configuration_generation = 0u, output_configuration_generation = 0u;
  uint32_t requested_width = 0u, requested_height = 0u, output_width = 0u, output_height = 0u;
  int32_t requested_scene_slot = -1, output_scene_slot = -1;
  int32_t requested_transfer_slot = -1, output_transfer_slot = -1;
  uint64_t producer_fence_b = 0u;
  bool staged = false;
  bool producer_wait_accepted = false;
};

inline DiOutputGate CheckDiOutputIdentity(const DiOutputIdentity& output) {
  if (!output.staged) return DiOutputGate::NotStaged;
  if (output.producer_fence_b == 0u || !output.producer_wait_accepted) return DiOutputGate::ProducerPending;
  if (output.requested_frame != output.output_frame || output.requested_owner_generation == 0u
      || output.requested_owner_generation != output.output_owner_generation
      || output.requested_scene_slot < 0 || output.requested_scene_slot != output.output_scene_slot
      || output.requested_transfer_slot < 0 || output.requested_transfer_slot != output.output_transfer_slot) {
    return DiOutputGate::StaleIdentity;
  }
  if (output.requested_width == 0u || output.requested_height == 0u || output.requested_width != output.output_width
      || output.requested_height != output.output_height
      || output.requested_configuration_generation == 0u
      || output.requested_configuration_generation != output.output_configuration_generation) {
    return DiOutputGate::ConfigurationMismatch;
  }
  return DiOutputGate::Ready;
}

inline bool CanReuseDiOutput(bool staged, uint64_t consumer_fence_c, uint64_t completed_fence_c) {
  return !staged || (consumer_fence_c != 0u && completed_fence_c != UINT64_MAX && completed_fence_c >= consumer_fence_c);
}

struct DiHistoryIdentity {
  uint32_t previous_frame = UINT32_MAX, current_frame = UINT32_MAX;
  uint32_t previous_width = 0u, previous_height = 0u, current_width = 0u, current_height = 0u;
  int32_t previous_transfer_slot = -1, current_transfer_slot = -1;
  uint64_t previous_owner_generation = 0u, current_owner_generation = 0u;
  uint64_t previous_configuration_generation = 0u, current_configuration_generation = 0u;
  bool motion_available = false;
  bool previous_inputs_available = false;
};

inline bool ValidDiHistoryIdentity(const DiHistoryIdentity& history) {
  return history.previous_inputs_available && history.motion_available && history.previous_frame != UINT32_MAX
         && history.previous_frame + 1u == history.current_frame && history.previous_width != 0u
         && history.previous_height != 0u && history.previous_width == history.current_width
         && history.previous_height == history.current_height && history.previous_transfer_slot >= 0
         && history.current_transfer_slot >= 0 && history.previous_transfer_slot != history.current_transfer_slot
         && history.previous_owner_generation != 0u && history.previous_owner_generation == history.current_owner_generation
         && history.previous_configuration_generation != 0u
         && history.previous_configuration_generation == history.current_configuration_generation;
}

}  // namespace falcom_world::rtxdi
